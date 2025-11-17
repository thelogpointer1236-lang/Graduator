#ifdef USE_STUB_IMPLEMENTATIONS

#include "core/services/ServiceLocator.h"

PressureSensor::PressureSensor(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_aboutToStop(false)
{
}

PressureSensor::~PressureSensor() {}

Pressure PressureSensor::getLastPressure() const {
    return m_lastPressure;
}

bool PressureSensor::openCOM(const QString &comPort, QString &error) {
    Q_UNUSED(comPort)
    Q_UNUSED(error)
    // Эмуляция успешного открытия
    ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"Заглушка: COM-порт %1 успешно открыт").arg(comPort));
    return true;
}

void PressureSensor::closeCOM() {
    ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"Заглушка: COM-порт закрыт"));
}

bool PressureSensor::isRunning() const {
    return m_isRunning;
}

void PressureSensor::stop() {
    m_aboutToStop = true;
    while (m_isRunning)
        QThread::msleep(10);
    m_aboutToStop = false;
}

void PressureSensor::start() {
    if (m_isRunning)
        return;

    m_isRunning = true;
    m_aboutToStop = false;
    ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"Заглушка: опрос датчика запущен"));

    const qreal totalUpTime = 50.0;    // сек
    const qreal totalDownTime = 50.0;  // сек
    const qreal maxPressure = 260.0;   // кгс
    const qreal period = totalUpTime + totalDownTime; // полный цикл = 40 сек

    QElapsedTimer timer;
    timer.start();

    while (!m_aboutToStop && !QThread::currentThread()->isInterruptionRequested()) {
        qreal elapsedSec = timer.elapsed() / 1000.0;
        qreal t = fmod(elapsedSec, period);

        qreal pressureValue;
        if (t < totalUpTime)
            pressureValue = (t / totalUpTime) * maxPressure;
        else
            pressureValue = maxPressure * (1.0 - (t - totalUpTime) / totalDownTime);

        const qreal timestampSec = ServiceLocator::instance().graduationService()->getElapsedTimeSeconds();
        m_lastPressure = Pressure(pressureValue, PressureUnit::Kgf);
        emit pressureMeasured(timestampSec, m_lastPressure);

        QThread::msleep(100); // 10 Гц опроса
    }

    m_isRunning = false;
    ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"Заглушка: опрос датчика остановлен"));
}

bool PressureSensor::pollOnce(QString &error) {
    Q_UNUSED(error)
    // В заглушке не используется
    return true;
}

#endif
