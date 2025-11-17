#include "GraduationService.h"

#include <iostream>
#include <QMessageBox>
#include "core/services/ServiceLocator.h"
#include <QThread>
bool isNearToPressureNode(const std::vector<double> &nodes, double p, double percentThreshold) {
    if (nodes.size() < 2) return false;
    double step = nodes[1] - nodes[0];
    for (const auto &node: nodes) {
        double diff = std::abs(p - node);
        double threshold = std::abs(step * percentThreshold / 100.0);
        if (diff <= threshold) return true;
    }
    return false;
}
GraduationService::GraduationService(QObject *parent) : QObject(parent) {
}
GraduationService::~GraduationService() {
    stop();
}
void GraduationService::start() {
    if (m_isRunning) return;
    stop();
    m_isRunning = true;
    m_forward = true;
    m_forwardGraduator.setPressureNodes(m_gaugeModel.pressureValues());
    m_backwardGraduator.setPressureNodes(m_gaugeModel.pressureValues());
    connectObjects();
    ServiceLocator::instance().cameraProcessor()->startAll();
    QMetaObject::invokeMethod(ServiceLocator::instance().pressureController(), "start", Qt::QueuedConnection);
    m_elapsedTimer.start();
}
void GraduationService::stop() {
    if (!m_isRunning) return;
    clearCurrentResult();
    m_currentResult.clear();
    m_forwardGraduator.clear();
    m_backwardGraduator.clear();
    m_elapsedTimer.invalidate();
    ServiceLocator::instance().cameraProcessor()->stopAll();
    ServiceLocator::instance().pressureController()->interrupt();
    disconnectObjects();
    m_isRunning = false;
}
bool GraduationService::isRunning() const {
    return m_isRunning;
}
bool GraduationService::isReadyToRun(QString &err) const {
    if (this->isRunning()) return false;
    const int gaugeIdx = ServiceLocator::instance().configManager()->get<int>("current.gaugeModel", -1);
    if (gaugeIdx < 0 || gaugeIdx >= ServiceLocator::instance().gaugeCatalog()->all().size()) {
        ServiceLocator::instance().logger()->error(L"Не задана модель манометра.");
        return false;
    }
    m_gaugeModel = ServiceLocator::instance().gaugeCatalog()->all().at(gaugeIdx);
    m_pressureUnit = static_cast<PressureUnit>(ServiceLocator::instance().configManager()->get<int>(
        "current.pressureUnit", static_cast<int>(PressureUnit::Unknown)));
    if (m_pressureUnit == PressureUnit::Unknown) {
        ServiceLocator::instance().logger()->error(L"Не задана единица измерения.");
        return false;
    }
    ServiceLocator::instance().pressureController()->setGaugePressurePoints(m_gaugeModel.pressureValues());
    ServiceLocator::instance().pressureController()->setPressureUnit(m_pressureUnit);
    if (!ServiceLocator::instance().pressureSensor()->isRunning()) {
        err = QString::fromWCharArray(L"Датчик давления не запущен.");
        return false;
    }
    if (!ServiceLocator::instance().pressureController()->isReadyToStart(err)) {
        return false;
    }
    return true;
}
std::vector<std::vector<double> > GraduationService::graduateForward() {
    m_currentResult.forward = m_forwardGraduator.graduate(3, 6);
    emitResultChangedIfNeeded();
    return m_currentResult.forward;
}
std::vector<std::vector<double> > GraduationService::graduateBackward() {
    m_currentResult.backward = m_backwardGraduator.graduate(3, 6);
    emitResultChangedIfNeeded();
    return m_currentResult.backward;
}
void GraduationService::switchToBackward() {
    m_forward = false;
}
void GraduationService::switchToForward() {
    m_forward = true;
}
qreal GraduationService::getElapsedTimeSeconds() const {
    if (!m_elapsedTimer.isValid()) return 0.0;
    return m_elapsedTimer.elapsed() / 1000.0;
}
PartyResult GraduationService::currentResult() const {
    return m_currentResult;
}

bool GraduationService::hasResult() const {
    return m_currentResult.isValid();
}

void GraduationService::setResult(const PartyResult &result) {
    m_currentResult = result;
    emitResultChangedIfNeeded();
}

void GraduationService::pushPressure(qreal t, qreal p) {
    if (m_forward) {
        m_forwardGraduator.pushPressure(t, p);
    } else {
        m_backwardGraduator.pushPressure(t, p);
    }
}
void GraduationService::pushAngle(qint32 i, qreal t, qreal a) {
    if (m_forward) {
        m_forwardGraduator.pushAngle(i, t, a);
    } else {
        m_backwardGraduator.pushAngle(i, t, a);
    }
}
void GraduationService::connectObjects() {
    connect(ServiceLocator::instance().cameraProcessor(), &CameraProcessor::angleMeasured,
            this, &GraduationService::onAngleMeasured, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureSensor(), &PressureSensor::pressureMeasured,
            this, &GraduationService::onPressureMeasured, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureController(), &PressureControllerBase::interrupted,
            this, &GraduationService::onInterrupted, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureController(), &PressureControllerBase::successfullyStopped,
            this, &GraduationService::onSuccessfullyStopped, Qt::QueuedConnection);
}
void GraduationService::disconnectObjects() {
    disconnect(ServiceLocator::instance().cameraProcessor(), nullptr, this, nullptr);
    disconnect(ServiceLocator::instance().pressureSensor(), nullptr, this, nullptr);
}

void GraduationService::clearCurrentResult() {
    m_currentResult.clear();
    emitResultChangedIfNeeded();
}

void GraduationService::emitResultChangedIfNeeded() {
    const bool state = hasResult();
    if (state != m_lastResultState) {
        m_lastResultState = state;
        emit currentResultChanged(state);
    }
}


// SLOTS:

void GraduationService::onPressureMeasured(qreal t, Pressure p) {
    if (!m_isRunning) return;
    ServiceLocator::instance().pressureController()->updatePressure(t, p.getValue(m_pressureUnit));
    qreal pressure = p.getValue(m_pressureUnit);
    m_isNearToPressurePoint = isNearToPressureNode(m_gaugeModel.pressureValues(), pressure, 10.0);
    if (m_isNearToPressurePoint) {
        pushPressure(t, pressure);
    }
}
void GraduationService::onAngleMeasured(qint32 i, qreal t, qreal a) {
    if (!m_isRunning) return;
    if (m_isNearToPressurePoint) {
        pushAngle(i, t, a);
    }
}

void GraduationService::onSuccessfullyStopped() {
    m_currentResult.forward = graduateForward();
    if (m_backwardGraduator.isEmpty()) {
        m_currentResult.backward = m_currentResult.forward;
    }
}

void GraduationService::onInterrupted() {
    stop();
}
