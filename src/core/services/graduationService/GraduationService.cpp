#include "GraduationService.h"
#include "core/services/ServiceLocator.h"
#include <QThread>

GraduationService::GraduationService(QObject *parent) :
    QObject(parent),
    m_graduator(8) {

}

GraduationService::~GraduationService() {

}

void GraduationService::start() {
    if (m_isRunning) return;
    m_isRunning = true;
    clearCurrentResult();
    m_graduator.setNodePressures(m_gaugeModel.pressureValues());
    m_graduator.setPressureWindow(m_gaugeModel.pressureValues()[1] * 0.075); // 7.5%
    m_graduator.setMinPoints(7);
    m_graduator.setLoessFrac(0.3);
    m_graduator.switchToForward();
    connectObjects();
    QMetaObject::invokeMethod(ServiceLocator::instance().pressureController(), "start", Qt::QueuedConnection);
    m_elapsedTimer.start();
    emit started();
}



bool GraduationService::isRunning() const {
    return m_isRunning;
}

bool GraduationService::isReadyToRun(QString &err) const {
    if (this->isRunning()) return false;
    const int gaugeIdx = ServiceLocator::instance().configManager()->get<int>("current.gaugeModel", -1);
    if (gaugeIdx < 0 || gaugeIdx >= ServiceLocator::instance().gaugeCatalog()->all().size()) {
        err =  QString::fromWCharArray(L"Не задана модель манометра.");
        return false;
    }
    m_gaugeModel = ServiceLocator::instance().gaugeCatalog()->all().at(gaugeIdx);
    if (m_gaugeModel.pressureValues().size() < 2) {
        err =  QString::fromWCharArray(L"Модель манометра некорректна.");
        return false;
    }
    m_pressureUnit = static_cast<PressureUnit>(ServiceLocator::instance().configManager()->get<int>(
        "current.pressureUnit", static_cast<int>(PressureUnit::Unknown)));
    if (m_pressureUnit == PressureUnit::Unknown) {
        err =  QString::fromWCharArray(L"Не задана единица измерения.");
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

grad::Graduator & GraduationService::graduator() {
    return m_graduator;
}

bool GraduationService::isResultReady() const {
    return m_resultReady;
}

const PartyResult& GraduationService::getPartyResult() {
    return m_currentResult;
}

qreal GraduationService::getElapsedTimeSeconds() const {
    if (!m_elapsedTimer.isValid()) return 0.0;
    return m_elapsedTimer.elapsed() / 1000.0;
}

void GraduationService::connectObjects() {
    connect(ServiceLocator::instance().cameraProcessor(), &CameraProcessor::angleMeasured,
            this, &GraduationService::onAngleMeasured, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureSensor(), &PressureSensor::pressureMeasured,
            this, &GraduationService::onPressureMeasured, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureController(), &PressureControllerBase::interrupted,
            this, &GraduationService::onControllerInterrupted, Qt::QueuedConnection);
    connect(ServiceLocator::instance().pressureController(), &PressureControllerBase::successfullyStopped,
            this, &GraduationService::onControllerSuccessfullyStopped, Qt::QueuedConnection);
}
void GraduationService::disconnectObjects() {
    disconnect(ServiceLocator::instance().cameraProcessor(), nullptr, this, nullptr);
    disconnect(ServiceLocator::instance().pressureSensor(), nullptr, this, nullptr);
    disconnect(ServiceLocator::instance().pressureController(), nullptr, this, nullptr);
}

void GraduationService::clearCurrentResult() {
    m_resultReady = false;
    m_graduator.clear();
    m_elapsedTimer.invalidate();
    emit resultCleared();
}


// SLOTS:
void GraduationService::onPressureMeasured(qreal t, Pressure p) {
    if (!m_isRunning) return;
    ServiceLocator::instance().pressureController()->updatePressure(t, p.getValue(m_pressureUnit));
    qreal pressure = p.getValue(m_pressureUnit);
    m_graduator.addPressureSample(t, pressure);
}
void GraduationService::onAngleMeasured(qint32 i, qreal t, qreal a) {
    if (!m_isRunning) return;
    m_graduator.addAngleSample(i, t, a);
}

void GraduationService::stop() {
    if (!m_isRunning) return;
    m_elapsedTimer.invalidate();
    disconnectObjects();
    ServiceLocator::instance().pressureController()->interrupt();
    m_isRunning = false;
}

void GraduationService::onControllerSuccessfullyStopped() {
    updateResult();
    disconnectObjects();
    ServiceLocator::instance().pressureController()->interrupt();
    emit resultReady();
}

void GraduationService::onControllerInterrupted() {
    m_elapsedTimer.invalidate();
    disconnectObjects();
    m_isRunning = false;
    emit interrupted();
}

void GraduationService::updateResult() {
    m_currentResult.gaugeModel = m_gaugeModel;
    m_currentResult.forward = m_graduator.graduateForward();
    m_currentResult.debugDataForward = m_graduator.allDebugDataForward();
    if (!m_graduator.isForward()) {
        m_currentResult.backward = m_graduator.graduateBackward();
        m_currentResult.debugDataBackward = m_graduator.allDebugDataBackward();
    }
    else {
        m_currentResult.backward = m_currentResult.forward;
        m_currentResult.debugDataBackward = m_currentResult.debugDataForward;
    }
    m_currentResult.durationSeconds = getElapsedTimeSeconds();
    m_resultReady = true;
}
