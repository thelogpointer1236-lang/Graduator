#include "PressureControllerStand5.h"
#include "core/services/ServiceLocator.h"
#include "defines.h"
#include <QThread>

#define interrupt   m_isRunning = false; \
                    emit interrupted(); \
                    return;

PressureControllerStand5::PressureControllerStand5(QObject *parent) : PressureControllerBase(parent) {
}

PressureControllerStand5::~PressureControllerStand5() {
}

void PressureControllerStand5::setMode(int modeIdx) {
}

QStringList PressureControllerStand5::getModes() const {
    return {
        tr("Прицел")
    };
}

qreal PressureControllerStand5::getTargetPressure() const {
    if (gaugePressureValues().size() < 2) { return 0.0; }
    return gaugePressureValues().back() * 1.02; // Целевое давление — последнее значение из шкалы + 2%
}
qreal PressureControllerStand5::getTargetPressureVelocity() const {
    return 0.0; // Для стенда 5 скорость не используется
}
void PressureControllerStand5::onPressureUpdated(qreal time, qreal pressure) {
    Q_UNUSED(time)
    Q_UNUSED(pressure)
}

qreal PressureControllerStand5::preloadFactor() const {
    return 0.5;
}

// Набор предварительного давления
void PressureControllerStand5::preloadPressure() {
    const qreal p_preload = getPreloadPressure();
    g540Driver()->setFlapsState(G540FlapsState::OpenInput);
    while (currentPressure() < p_preload) {
        if (shouldStop()) {
            g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
            interrupt;
        }
        QThread::msleep(15);
    }
    g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
}
void PressureControllerStand5::forwardPressure() {
    // Движение поршня вперед до целевого давления
    const qreal p_target = getTargetPressure();
    g540Driver()->setDirection(G540Direction::Forward);
    while (currentPressure() < p_target) {
        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }
        g540Driver()->setFrequency(g540Driver()->maxFrequency());
        QThread::msleep(90);
    }
}
void PressureControllerStand5::backwardPressure() {
    // Движение поршня назад до концевика
    g540Driver()->setDirection(G540Direction::Backward);
    while (!isStartLimitTriggered()) {
        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }
        g540Driver()->setFrequency(g540Driver()->maxFrequency());
        QThread::msleep(90);
    }
    g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
}
qreal PressureControllerStand5::getPreloadPressure() const {
    return gaugePressureValues()[1] * preloadFactor();
}
void PressureControllerStand5::start() {
    m_isRunning = true;
    emit started();
    auto *gs = ServiceLocator::instance().graduationService();
    gs->graduator().switchToForward();
    preloadPressure(); // 1
    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    forwardPressure(); // 2
    gs->graduator().switchToBackward();
    backwardPressure(); // 3
    g540Driver()->stop();
    m_isRunning = false;
}
bool PressureControllerStand5::isReadyToStart(QString &err) const {
    if (!PressureControllerBase::isReadyToStart(err)) return false;
    if (gaugePressureValues().size() < 2) {
        err = tr("Недостаточно точек шкалы давления для работы контроллера.");
        return false;
    }
    if (currentPressure() > getPreloadPressure()) {
        err = tr("Текущее давление превысило давление преднагрузки.");
        return false;
    }
    return true;
}
