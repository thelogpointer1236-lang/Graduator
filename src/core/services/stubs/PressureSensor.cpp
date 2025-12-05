#ifdef USE_STUB_IMPLEMENTATIONS

#include "core/services/PressureSensor.h"
#include "core/services/ServiceLocator.h"
#include "core/types/Pressure.h"

#include <QRandomGenerator>
#include <QTimer>

PressureSensor::PressureSensor(QObject *parent)
    : QObject(parent) {
    m_lastPressure = Pressure::fromKPa(0.0);
}

PressureSensor::~PressureSensor() {
    stop();
}

Pressure PressureSensor::getLastPressure() const {
    return m_lastPressure;
}

bool PressureSensor::openCOM(const QString &comPort, QString &error) {
    m_comPort = comPort;
    error.clear();
    if (auto *logger = ServiceLocator::instance().logger()) {
        logger->info(QString::fromWCharArray(L"Используется заглушка датчика давления для порта %1").arg(m_comPort));
    }
    return true;
}

bool PressureSensor::isRunning() const {
    return m_isRunning;
}

void PressureSensor::stop() {
    m_aboutToStop = true;
    m_isRunning = false;
}

void PressureSensor::closeCOM() {
    m_comPort.clear();
}

void PressureSensor::start() {
    if (m_isRunning) {
        return;
    }

    m_aboutToStop = false;
    m_isRunning = true;

    const auto emitReading = [this]() {
        if (!m_isRunning || m_aboutToStop) {
            m_isRunning = false;
            m_aboutToStop = false;
            return;
        }

        const double basePressureKPa = 100.0;
        const double noise = QRandomGenerator::global()->bounded(-5.0, 5.0);
        m_lastPressure = Pressure::fromKPa(basePressureKPa + noise);

        qreal timestampSec = 0.0;
        if (auto *graduationService = ServiceLocator::instance().graduationService()) {
            timestampSec = graduationService->getElapsedTimeSeconds();
        }

        emit pressureMeasured(timestampSec, m_lastPressure);

        QTimer::singleShot(100, this, emitReading);
    };

    QTimer::singleShot(0, this, emitReading);
}

bool PressureSensor::pollOnce(QString &error) {
    Q_UNUSED(error);
    return true;
}

#endif
