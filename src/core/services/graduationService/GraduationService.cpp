#include "GraduationService.h"
#include "core/services/ServiceLocator.h"
#include "core/services/pressureController/impl/utils.h"
#include <QThread>
#include <QDebug>

#include <algorithm>

#include "defines.h"

namespace {
    constexpr qreal kSensorTimeoutSec = 2.5;
    constexpr int   kWatchdogIntervalMs = 500;
}

GraduationService::GraduationService(QObject *parent)
    : QObject(parent),
      m_graduator(8),
      m_watchdogTimer(this)
{
    m_watchdogTimer.setInterval(kWatchdogIntervalMs);
    connect(&m_watchdogTimer, &QTimer::timeout,
            this, &GraduationService::checkSensorsActivity);
}

GraduationService::~GraduationService() {
    disconnectObjects();
}

// ======================================================
// 1. PREPARE PHASE
// ======================================================

bool GraduationService::prepare(QString *err_)
{
    auto* pc = ServiceLocator::instance().pressureController();
    auto* ps = ServiceLocator::instance().pressureSensor();
    auto& cfg = *ServiceLocator::instance().configManager();
    auto& catalog = *ServiceLocator::instance().gaugeCatalog();

    QString err;

    const int gaugeIdx = cfg.get<int>(CFG_KEY_CURRENT_GAUGE_MODEL, -1);
    if (gaugeIdx < 0 || gaugeIdx >= catalog.all().size()) {
        err = tr("Gauge model is not specified.");
        goto fail;
    }

    m_gaugeModel = catalog.all()[gaugeIdx];
    if (m_gaugeModel.pressureValues().size() < 2) {
        err = tr("Gauge model is invalid.");
        goto fail;
    }

    m_pressureUnit = static_cast<PressureUnit>(
        cfg.get<int>(CFG_KEY_CURRENT_PRESSURE_UNIT,
                     static_cast<int>(PressureUnit::Unknown))
    );
    if (m_pressureUnit == PressureUnit::Unknown) {
        err = tr("Pressure unit is not specified.");
        goto fail;
    }

    if (!ps) {
        err = tr("Pressure sensor is not available.");
        goto fail;
    }

    if (!ps->isRunning()) {
        err = tr("Pressure sensor is not running.");
        goto fail;
    }

    if (!pc) {
        err = tr("Pressure controller is not available.");
        goto fail;
    }

    const auto& pressureValues = m_gaugeModel.pressureValues();
    const auto maxPressureIt =
        std::max_element(pressureValues.begin(), pressureValues.end());

    if (maxPressureIt == pressureValues.end() || *maxPressureIt <= 0.0) {
        err = tr("Gauge pressure range is invalid.");
        goto fail;
    }

    pc->setGaugePressurePoints(pressureValues);
    pc->setPressureUnit(m_pressureUnit);

    const double currentPressure = ps->getLastPressure().getValue(m_pressureUnit);

    if (!std::isfinite(currentPressure)) {
        err = tr("No valid pressure data available.");
        goto fail;
    }

    pc->updatePressure(0, currentPressure);

    if (qFuzzyIsNull(pc->preloadFactor())) {
        err = tr("Preload factor is not set.");
        goto fail;
    }

    if (!pc->isReadyToStart(err)) {
        goto fail;
    }

    m_state = State::Prepared;
    if (err_) err_->clear();
    return true;

fail:
    if (err_) *err_ = err;
    return false;
}


// ======================================================
// 2. START PHASE
// ======================================================

bool GraduationService::start()
{
    if (m_state != State::Prepared)
        return false;

    clearForNewRun();
    connectObjects();

    m_graduator.setNodePressures(m_gaugeModel.pressureValues());
    m_graduator.setPressureWindow(m_gaugeModel.pressureValues()[1] * 0.075);
    m_graduator.setMinPoints(7);
    m_graduator.setLoessFrac(0.3);
    m_graduator.switchToForward();

    QMetaObject::invokeMethod(
        ServiceLocator::instance().pressureController(),
        "start",
        Qt::QueuedConnection
    );

    m_state = State::Running;
    m_elapsedTimer.start();
    m_lastPressureTimestamp = getElapsedTimeSeconds();
    for (auto& [idx, _] : m_lastAngleTimestamps) {
        m_lastAngleTimestamps[idx] = m_lastPressureTimestamp;
    }
    m_watchdogTimer.start();
    emit started();
    return true;
}

// ======================================================
// 3. INTERRUPT
// ======================================================

void GraduationService::interrupt()
{
    if (m_state != State::Running)
        return;

    auto* pc = ServiceLocator::instance().pressureController();
    stopWatchdog();
    disconnectObjects();
    pc->interrupt();

    m_state = State::Idle;
    m_elapsedTimer.invalidate();
    clearResultOnly(); // НЕ вызов updateResult!

    emit interrupted();
    m_state = State::Idle;
}

// ======================================================
// 4. FINALIZE / GET RESULT
// ======================================================

void GraduationService::onPressureControllerResultReady()
{
    if (m_state != State::Running)
        return;

    stopWatchdog();
    disconnectObjects();

    m_elapsedTimer.invalidate();

    updateResult();
    m_state = State::Idle;

    emit ended();
    emit resultAvailabilityChanged(true);

    m_state = State::Idle;
}

int GraduationService::angleMeasCountForCamera(qint32 idx, bool isForward) const {
    decltype(auto) ac = isForward ? m_graduator.anglesCountForward() : m_graduator.anglesCountBackward();
    if (idx >= ac.size()) return 0;
    return ac[idx];
}

// ======================================================
// 5. EVENT HANDLERS
// ======================================================

void GraduationService::onPressureMeasured(qreal t, Pressure p)
{
    if (m_state != State::Running)
        return;

    qreal val = p.getValue(m_pressureUnit);
    ServiceLocator::instance().pressureController()->updatePressure(t, val);
    m_graduator.addPressureSample(t, val);
    m_lastPressureTimestamp = t;
}

void GraduationService::onAngleMeasured(qint32 idx, qreal t, qreal a)
{
    if (m_state != State::Running)
        return;

    m_graduator.addAngleSample(idx, t, a);
    m_lastAngleTimestamps[idx] = t;
}

void GraduationService::checkSensorsActivity()
{
    if (m_state != State::Running)
        return;

    const qreal now = getElapsedTimeSeconds();
    const bool pressureStalled = (now - m_lastPressureTimestamp) > kSensorTimeoutSec;

    bool anyCameraStalled = false;
    int stalledCameraIdx = -1;

    for (auto& [idx, _] : m_lastAngleTimestamps) {
        if ((now - m_lastAngleTimestamps[idx]) > kSensorTimeoutSec) {
            anyCameraStalled = true;
            stalledCameraIdx = idx;
            break;
        }
    }

    if (!pressureStalled && !anyCameraStalled)
        return;

    QString reason;
    if (pressureStalled && anyCameraStalled) {
        reason = tr("Pressure and camera %1 have stopped being measured.").arg(stalledCameraIdx + 1);
    } else if (pressureStalled) {
        reason = tr("Pressure has stopped being measured.");
    } else {
        reason = tr("Camera %1 has stopped being measured.").arg(stalledCameraIdx + 1);
    }

    interrupt();

    ServiceLocator::instance().userDialogService()
        ->requestUserInput(tr("The Graduation process has been interrupted"), reason, {});
}

void GraduationService::onPressureControllerInterrupted()
{
    interrupt(); // already emits interrupted()
}

// ======================================================
// INTERNAL HELPERS
// ======================================================

void GraduationService::connectObjects()
{
    auto& loc = ServiceLocator::instance();

    connect(loc.cameraProcessor(), &CameraProcessor::angleMeasured,
            this, &GraduationService::onAngleMeasured, Qt::QueuedConnection);

    connect(loc.pressureSensor(), &PressureSensor::pressureMeasured,
            this, &GraduationService::onPressureMeasured, Qt::QueuedConnection);

    connect(loc.pressureController(), &PressureControllerBase::interrupted,
            this, &GraduationService::onPressureControllerInterrupted, Qt::QueuedConnection);

    connect(loc.pressureController(), &PressureControllerBase::resultReady,
            this, &GraduationService::onPressureControllerResultReady, Qt::QueuedConnection);
}

void GraduationService::disconnectObjects()
{
    auto& loc = ServiceLocator::instance();
    disconnect(loc.cameraProcessor(), nullptr, this, nullptr);
    disconnect(loc.pressureSensor(), nullptr, this, nullptr);
    disconnect(loc.pressureController(), nullptr, this, nullptr);
}

void GraduationService::clearForNewRun()
{
    stopWatchdog();
    m_resultReady = false;
    m_currentResult = PartyResult{};
    m_graduator.clear();
    m_elapsedTimer.invalidate();
    m_lastPressureTimestamp = 0.0;
    for (auto& [idx, _] : m_lastAngleTimestamps) {
        m_lastAngleTimestamps[idx] = 0.0;
    }
    emit tableUpdateRequired();
    emit resultAvailabilityChanged(false);
}

void GraduationService::clearResultOnly()
{
    stopWatchdog();
    m_resultReady = false;
    m_currentResult = PartyResult{};
    m_graduator.clear();
    emit resultAvailabilityChanged(false);
}

void GraduationService::stopWatchdog()
{
    if (m_watchdogTimer.isActive())
        m_watchdogTimer.stop();
}

// ======================================================
// PUBLIC API
// ======================================================

bool GraduationService::isResultReady() const {
    return m_resultReady;
}

const PartyResult& GraduationService::getPartyResult() const {
    return m_currentResult;
}

void GraduationService::setStrongNode(bool strong) {
    m_strongNode = strong;
    m_currentResult.strongNode = strong;
}

qreal GraduationService::getElapsedTimeSeconds() const {
    return m_elapsedTimer.isValid()
            ? m_elapsedTimer.elapsed() / 1000.0
            : 0.0;
}

void GraduationService::requestUpdateResultAndTable()
{
    emit tableUpdateRequired();
}






namespace {
    static std::vector<double> computeNonlinearity(
        const std::vector<std::vector<NodeResult>> &allCams)
    {
        std::vector<double> result;
        result.reserve(allCams.size());

        for (auto &cam : allCams) {
            const size_t n = cam.size();
            if (n < 2) {
                result.push_back(std::numeric_limits<double>::quiet_NaN());
                continue;
            }

            // avrDelta = (α_n − α_1) / (n − 1)
            double a1 = cam.front().angle;
            double an = cam.back().angle;
            double avrDelta = (an - a1) / double(n - 1);

            // maxD = max | (α_{i+1} − α_i) − avrDelta |
            double maxD = 0.0;
            for (size_t i = 0; i + 1 < n; ++i) {
                double delta = cam[i + 1].angle - cam[i].angle;
                double d = std::abs(delta - avrDelta);
                if (d > maxD) maxD = d;
            }

            double NL = (avrDelta != 0.0)
                            ? (maxD / std::abs(avrDelta)) * 100.0
                            : std::numeric_limits<double>::quiet_NaN();

            result.push_back(NL);
        }

        return result;
    }

}


void GraduationService::updateResult()
{
    m_currentResult = PartyResult{};
    m_currentResult.gaugeModel = m_gaugeModel;
    m_currentResult.strongNode = m_strongNode;
    if (auto *partyManager = ServiceLocator::instance().partyManager()) {
        m_currentResult.precisionClass = partyManager->currentPrecisionValue();
    }

    auto fwd = m_graduator.graduateForward();
    auto dbgFwd = m_graduator.allDebugDataForward();

    auto back = m_graduator.isForward()
              ? fwd
              : m_graduator.graduateBackward();

    auto dbgBack = m_graduator.isForward()
                 ? dbgFwd
                 : m_graduator.allDebugDataBackward();

    m_currentResult.forward          = std::move(fwd);
    m_currentResult.backward         = std::move(back);
    m_currentResult.debugDataForward = std::move(dbgFwd);
    m_currentResult.debugDataBackward= std::move(dbgBack);

    // Коррекция forward/backward
    size_t cams = std::min(m_currentResult.forward.size(),
                           m_currentResult.backward.size());
    for (size_t cam = 0; cam < cams; ++cam) {
        auto &fw  = m_currentResult.forward[cam];
        auto &bk  = m_currentResult.backward[cam];
        if (!fw.empty() && !bk.empty()) {
            bk[0] = fw[0];
            if (!qIsFinite(bk.back().angle))
                bk.back() = fw.back();
        }
    }

    // Нелинейность
    m_currentResult.nolinForward  = computeNonlinearity(m_currentResult.forward);
    if (m_currentResult.backward[0].size() == m_gaugeModel.pressureValues().size())
        m_currentResult.nolinBackward = computeNonlinearity(m_currentResult.backward);

    m_currentResult.durationSeconds = getElapsedTimeSeconds();
    m_resultReady = true;

    requestUpdateResultAndTable();
}

