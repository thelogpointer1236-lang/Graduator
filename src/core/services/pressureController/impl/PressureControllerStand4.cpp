#include "PressureControllerStand4.h"
#include <QThread>
#include "core/services/ServiceLocator.h"
#include "utils.h"

#define interrupt   m_isRunning = false; \
emit interrupted(); \
return;

#define MODE_INFERENCE 0
#define MODE_FORWARD 1
#define MODE_FORWARD_AND_BACKWARD 2

double calculateFrequency(double maxFrequency, double currentPressure, double limitPressure) {
    if (currentPressure >= limitPressure)
        return maxFrequency / 4.0;
    if (currentPressure <= 0)
        return maxFrequency;
    double ratio = currentPressure / limitPressure; // от 0 до 1
    double frequency = maxFrequency - (maxFrequency * 3.0 / 4.0) * ratio; // линейное уменьшение
    return frequency;
}

bool isNearToPressureNode(const std::vector<double> &nodes, double p, double percentThreshold);

PressureControllerStand4::PressureControllerStand4(QObject *parent) : PressureControllerBase(parent) {
}

PressureControllerStand4::~PressureControllerStand4() {
}

void PressureControllerStand4::setMode(int modeIdx) {
    m_currentMode = modeIdx;
}
QStringList PressureControllerStand4::getModes() const {
    return {
        QString::fromWCharArray(L"Прицел"),
        QString::fromWCharArray(L"Прямой ход"),
        QString::fromWCharArray(L"Прямой и обратный ход"),
    };
}

qreal PressureControllerStand4::getTargetPressure() const {
    if (gaugePressureValues().size() < 2) { return 0.0; }
    return gaugePressureValues().back() * 1.01; // Целевое давление — последнее значение из шкалы + 2%
}

qreal PressureControllerStand4::getTargetPressureVelocity() const {
    return m_dP_target;
}

bool PressureControllerStand4::isReadyToStart(QString &err) const {
    if (!PressureControllerBase::isReadyToStart(err)) return false;
    if (gaugePressureValues().size() < 2) {
        err = QString::fromWCharArray(L"Недостаточно точек шкалы давления для работы контроллера.");
        return false;
    }
    if (currentPressure() > getPreloadPressure()) {
        err = QString::fromWCharArray(L"Текущее давление превысило давление преднагрузки.");
        return false;
    }
    return true;
}

qreal PressureControllerStand4::getPreloadPressure() const {
    if (gaugePressureValues().size() < 2) { return 0.0; }
    return gaugePressureValues()[1] * getPreloadFactor();
}

qreal PressureControllerStand4::getNominalPressureVelocity() const {
    if (gaugePressureValues().size() < 2) { return 0.0; }
    return gaugePressureValues().back() / getNominalDurationSec();
}

qreal PressureControllerStand4::getMaxPressureVelocity() const {
    return getNominalPressureVelocity() * getMaxVelocityFactor();
}

qreal PressureControllerStand4::getMinPressureVelocity() const {
    return getNominalPressureVelocity() * getMinVelocityFactor();
}

void PressureControllerStand4::onPressureUpdated(qreal time, qreal pressure) {
    Q_UNUSED(time);
    Q_UNUSED(pressure);
}

// Набор предварительного давления
void PressureControllerStand4::preloadPressure() {
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

void PressureControllerStand4::forwardPressure() {
    // Движение поршня вперед до целевого давления
    const qreal p_target = getTargetPressure();
    const int f_max = g540Driver()->maxFrequency();
    g540Driver()->setDirection(G540Direction::Forward);
    while (currentPressure() < p_target) {
        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }
        const qreal p_cur = currentPressure();
        m_dP_target = getMaxPressureVelocity();
        int freq = calculateFrequency(f_max, p_cur, p_target);
        if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5)) {
            m_dP_target = getMinPressureVelocity();
            freq = freq / 3;
        }
        g540Driver()->setFrequency(freq);
        QThread::msleep(90);
    }
}

void PressureControllerStand4::backwardPressure() {
    // Движение поршня назад до концевика
    const qreal p_target = getTargetPressure();
    const int f_max = g540Driver()->maxFrequency();
    g540Driver()->setDirection(G540Direction::Backward);
    while (!isStartLimitTriggered()) {
        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }
        if (m_currentMode == MODE_FORWARD_AND_BACKWARD) {
            const qreal p_cur = currentPressure();
            m_dP_target = getMaxPressureVelocity();
            int freq = calculateFrequency(f_max, p_cur, p_target);
            if (p_cur <= getPreloadPressure()) {
                g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
            } else {
                g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
            }
            if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5)) {
                m_dP_target = getMinPressureVelocity();
                freq = freq / 3;
            }
            g540Driver()->setFrequency(freq);
        }
        else {
            g540Driver()->setFrequency(f_max);
        }
        QThread::msleep(90);
    }
    if (currentPressure() < getPreloadPressure()) {
        g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
    } else {
        g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
    }
}

void PressureControllerStand4::start() {
    m_isRunning = true;
    emit started();
    auto *gs = ServiceLocator::instance().graduationService();
    gs->switchToForward();
    preloadPressure(); // 1
    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    forwardPressure(); // 2
    gs->switchToBackward();
    backwardPressure(); // 3
    g540Driver()->stop();
    gs->stop();
    m_dP_target = 0;
    m_isRunning = false;
    emit successfullyStopped();
}
