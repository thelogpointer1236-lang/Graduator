#include "GraduationService.h"
#include "core/services/ServiceLocator.h"
#include <QThread>
#include <QDebug>

GraduationService::GraduationService(QObject *parent)
    : QObject(parent),
      m_graduator(8)
{
}

GraduationService::~GraduationService() {
    disconnectObjects();
}

// ======================================================
// 1. PREPARE PHASE
// ======================================================

bool GraduationService::prepare(QString &err)
{
    if (m_state != State::Idle) {
        err = QString::fromWCharArray(L"Сервис уже инициализирован.");
        return false;
    }

    auto* pc = ServiceLocator::instance().pressureController();
    auto* ps = ServiceLocator::instance().pressureSensor();
    auto& cfg = *ServiceLocator::instance().configManager();

    const int gaugeIdx = cfg.get<int>("current.gaugeModel", -1);
    auto& catalog = *ServiceLocator::instance().gaugeCatalog();

    if (gaugeIdx < 0 || gaugeIdx >= catalog.all().size()) {
        err = QString::fromWCharArray(L"Не задана модель манометра.");
        return false;
    }

    m_gaugeModel = catalog.all()[gaugeIdx];
    if (m_gaugeModel.pressureValues().size() < 2) {
        err = QString::fromWCharArray(L"Модель манометра некорректна.");
        return false;
    }

    m_pressureUnit = static_cast<PressureUnit>(
        cfg.get<int>("current.pressureUnit", static_cast<int>(PressureUnit::Unknown))
    );
    if (m_pressureUnit == PressureUnit::Unknown) {
        err = QString::fromWCharArray(L"Не задана единица давления.");
        return false;
    }

    if (!ps->isRunning()) {
        err = QString::fromWCharArray(L"Датчик давления не запущен.");
        return false;
    }

    pc->setGaugePressurePoints(m_gaugeModel.pressureValues());
    pc->setPressureUnit(m_pressureUnit);

    if (!pc->isReadyToStart(err))
        return false;

    m_state = State::Prepared;
    return true;
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
    disconnectObjects();
    pc->interrupt();

    m_state = State::Interrupted;
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

    disconnectObjects();

    updateResult();
    m_state = State::Finished;

    emit ended();
    emit resultAvailabilityChanged(true);

    m_state = State::Idle;
}

void GraduationService::updateResult()
{
    m_currentResult = PartyResult{};
    m_currentResult.gaugeModel = m_gaugeModel;

    m_currentResult.forward          = m_graduator.graduateForward();
    m_currentResult.debugDataForward = m_graduator.allDebugDataForward();

    if (!m_graduator.isForward()) {
        m_currentResult.backward          = m_graduator.graduateBackward();
        m_currentResult.debugDataBackward = m_graduator.allDebugDataBackward();
    } else {
        m_currentResult.backward          = m_currentResult.forward;
        m_currentResult.debugDataBackward = m_currentResult.debugDataForward;
    }

    // Коррекция forward/backward
    size_t cams = std::min(m_currentResult.forward.size(),
                           m_currentResult.backward.size());
    for (size_t cam = 0; cam < cams; ++cam) {
        auto &back = m_currentResult.backward[cam];
        auto &fwd  = m_currentResult.forward[cam];
        if (!back.empty() && !fwd.empty()) {
            back[0] = fwd[0];
            if (!qIsFinite(back.back().angle))
                back.back() = fwd.back();
        }
    }

    m_currentResult.durationSeconds = getElapsedTimeSeconds();
    m_resultReady = true;
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
}

void GraduationService::onAngleMeasured(qint32 idx, qreal t, qreal a)
{
    if (m_state != State::Running)
        return;

    m_graduator.addAngleSample(idx, t, a);
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
    m_resultReady = false;
    m_currentResult = PartyResult{};
    m_graduator.clear();
    m_elapsedTimer.invalidate();
    emit resultAvailabilityChanged(false);
}

void GraduationService::clearResultOnly()
{
    m_resultReady = false;
    m_currentResult = PartyResult{};
    emit resultAvailabilityChanged(false);
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

qreal GraduationService::getElapsedTimeSeconds() const {
    return m_elapsedTimer.isValid()
            ? m_elapsedTimer.elapsed() / 1000.0
            : 0.0;
}

void GraduationService::requestUpdateResultAndTable()
{
    // Разрешено обновлять таблицу только когда результат есть
    if (m_state == State::Finished && m_resultReady) {
        updateResult();
    }

    emit tableUpdateRequired();
}