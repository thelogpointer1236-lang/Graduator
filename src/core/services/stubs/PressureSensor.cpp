#ifdef USE_STUB_PRESSURE_IMPLEMENTATIONS

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
        logger->info(tr("Используется заглушка датчика давления для порта %1").arg(m_comPort));
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

void PressureSensor::start()
{
    if (m_isRunning) {
        return;
    }

    m_aboutToStop = false;
    m_isRunning = true;

    while (m_isRunning && !m_aboutToStop) {

        qreal timestampSec = 0.0;
        if (auto *graduationService = ServiceLocator::instance().graduationService()) {
            timestampSec = graduationService->getElapsedTimeSeconds();
        }

        // Период 60 секунд
        const double period = 60.0;
        const double halfPeriod = period / 2.0; // 30 секунд
        const double minPressure = -10.0;
        const double maxPressure = 266.0;

        double phase = std::fmod(timestampSec, period);

        double pressureKPa = 0.0;

        if (phase < halfPeriod) {
            // рост: -10 → 260
            pressureKPa = minPressure + (phase / halfPeriod) * (maxPressure - minPressure);
        } else {
            // спад: 260 → -10
            pressureKPa = minPressure + ((period - phase) / halfPeriod) * (maxPressure - minPressure);
        }


        // (Опционально) добавить шум ±5
        // pressureKPa += QRandomGenerator::global()->bounded(-5.0, 5.0);

        m_lastPressure = Pressure::fromKgf(pressureKPa);

        emit pressureMeasured(timestampSec, m_lastPressure);

        QThread::msleep(100);
    }

    m_isRunning = false;
    m_aboutToStop = false;
}



bool PressureSensor::pollOnce(QString &error) {
    Q_UNUSED(error);
    return true;
}

#endif
