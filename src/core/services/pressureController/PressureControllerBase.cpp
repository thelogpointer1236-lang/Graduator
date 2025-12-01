#include "PressureControllerBase.h"
#include "core/services/ServiceLocator.h"
#include <QMetaObject>
#include <QThread>

PressureControllerBase::PressureControllerBase(QObject *parent) : QObject(parent) {
    m_G540Driver = std::make_unique<G540Driver>();
    m_G540DriverThread = new QThread(this);
    m_G540Driver->moveToThread(m_G540DriverThread);
    m_G540DriverThread->start();
}

PressureControllerBase::~PressureControllerBase()
{
    if (m_isRunning) {
        interrupt();
    }

    if (m_G540Driver) {
        if (m_G540Driver->isRunning()) {
            m_G540Driver->stop();
        }
    }

    if (m_G540DriverThread) {

        // Удаляем объект в его собственном потоке
        if (m_G540Driver) {
            QMetaObject::invokeMethod(
                m_G540Driver.get(),
                "deleteLater",
                Qt::QueuedConnection
            );
        }

        m_G540DriverThread->quit();
        m_G540DriverThread->wait();

        delete m_G540DriverThread;
        m_G540DriverThread = nullptr;
    }

    m_G540Driver.reset();
}

void PressureControllerBase::setGaugePressurePoints(const std::vector<qreal> &pressureValues) {
    m_gaugePressureValues = pressureValues;
}
void PressureControllerBase::setPressureUnit(PressureUnit unit) {
    m_pressureUnit = unit;
}
void PressureControllerBase::updatePressure(qreal time, qreal pressure) {
    m_pressure_history.emplace_back(pressure);
    m_time_history.emplace_back(time);
    if (g540Driver()->isRunning()) {
        m_c_imp_history.emplace_back(g540Driver()->impulsesCount());
        m_freq_history.emplace_back(g540Driver()->frequency());
    }
    g540Driver()->updatePressure(pressure);
}
quint32 PressureControllerBase::getImpulsesCount() const {
    return m_G540Driver->impulsesCount();
}
quint32 PressureControllerBase::getImpulsesFreq() const {
    return m_G540Driver->frequency();
}
qreal PressureControllerBase::getCurrentPressureVelocity() const {
    if (m_pressure_history.size() < 2) {
        return 0.0;
    }
    const qreal dp = m_pressure_history.back() - m_pressure_history[m_pressure_history.size() - 2];
    const qreal dt = m_time_history.back() - m_time_history[m_time_history.size() - 2];
    if (dt < 0.0001) {
        return 0.0;
    }
    return dp / dt;
}
qreal PressureControllerBase::getCurrentPressure() const {
    if (m_pressure_history.empty()) {
        return 0.0;
    }
    return m_pressure_history.back();
}
qreal PressureControllerBase::getTimeSinceStartSec() const {
    return m_G540Driver->timeSinceStartSec();
}
bool PressureControllerBase::isStartLimitTriggered() const {
    return m_G540Driver->isStartLimitTriggered();
}
bool PressureControllerBase::isEndLimitTriggered() const {
    return m_G540Driver->isEndLimitTriggered();
}
bool PressureControllerBase::isAnyLimitTriggered() const {
    return m_G540Driver->isAnyLimitTriggered();
}
void PressureControllerBase::openInletFlap() {
    m_G540Driver->setFlapsState(G540FlapsState::OpenInput);
}
void PressureControllerBase::openOutletFlap() {
    m_G540Driver->setFlapsState(G540FlapsState::OpenOutput);
}
void PressureControllerBase::closeBothFlaps() {
    m_G540Driver->setFlapsState(G540FlapsState::CloseBoth);
}
bool PressureControllerBase::isReadyToStart(QString &err) const {
    if (QString e; !g540Driver()->isReadyToStart(e)) {
        err = QString::fromWCharArray(L"Контроллер G540 не готов к работе.\n") + e;
        ServiceLocator::instance().logger()->error(err);
        return false;
    }
    return true;
}
bool PressureControllerBase::isRunning() const {
    return m_isRunning;
}
void PressureControllerBase::interrupt() {
    if (!m_isRunning && !m_aboutToStop) {
        return;
    }
    m_aboutToStop = true;
    while (m_isRunning) {
        QThread::msleep(10);
    }
    m_aboutToStop = false;
    emit interrupted();
}
const std::vector<qreal> &PressureControllerBase::gaugePressureValues() const {
    return m_gaugePressureValues;
}
PressureUnit PressureControllerBase::pressureUnit() const {
    return m_pressureUnit;
}
qreal PressureControllerBase::currentPressure() const {
    if (m_pressure_history.empty()) {
        return 0.0;
    }
    return m_pressure_history.back();
}
G540Driver *PressureControllerBase::g540Driver() const { return m_G540Driver.get(); }
bool PressureControllerBase::shouldStop() const noexcept {
    return QThread::currentThread()->isInterruptionRequested() || m_aboutToStop;
}
void PressureControllerBase::startGoToEnd() {
    m_isRunning = true;
    emit started();
    g540Driver()->setDirection(G540Direction::Forward);
    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    while (!g540Driver()->isEndLimitTriggered()) {
        if (shouldStop()) break;
        g540Driver()->setFrequency(g540Driver()->maxFrequency());
        QThread::msleep(90);
    }
    g540Driver()->stop();
    m_isRunning = false;
    emit interrupted();
}
void PressureControllerBase::startGoToStart() {
    m_isRunning = true;
    emit started();
    g540Driver()->setDirection(G540Direction::Backward);
    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    const auto emergencyStopIfBothLimitsTriggered = [this]() -> bool {
        if (isStartLimitTriggered() && isEndLimitTriggered()) {
            ServiceLocator::instance().logger()->error(
                QString::fromWCharArray(L"Обнаружено одновременное срабатывание концевых выключателей. Выполнена экстренная остановка."));
            g540Driver()->stop();
            m_isRunning = false;
            emit interrupted();
            return true;
        }
        return false;
    };
    if (emergencyStopIfBothLimitsTriggered()) {
        return;
    }
    while (!isStartLimitTriggered()) {
        if (shouldStop()) break;
        if (emergencyStopIfBothLimitsTriggered()) {
            return;
        }
        g540Driver()->setFrequency(g540Driver()->maxFrequency());
        QThread::msleep(90);
    }
    if (emergencyStopIfBothLimitsTriggered()) {
        return;
    }
    g540Driver()->stop();
    m_isRunning = false;
    emit interrupted();
}
