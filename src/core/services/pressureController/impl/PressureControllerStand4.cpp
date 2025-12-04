#include "PressureControllerStand4.h"
#include <QThread>
#include <QDebug>
#include "core/services/ServiceLocator.h"
#include "utils.h"

#define interrupt   m_isRunning = false; \
CameraProcessor::restoreDefaultCapRate(); \
emit interrupted(); \
return;

#define MODE_INFERENCE 0
#define MODE_FORWARD 1
#define MODE_FORWARD_AND_BACKWARD 2

namespace {
    double calculateFrequency(double maxFrequency, double currentPressure, double limitPressure) {
        if (currentPressure >= limitPressure)
            return maxFrequency / 4.0;
        if (currentPressure <= 0)
            return maxFrequency;
        double ratio = currentPressure / limitPressure; // от 0 до 1
        double frequency = maxFrequency - (maxFrequency * 3.0 / 4.0) * ratio; // линейное уменьшение
        return frequency;
    }

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
}

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
        const qreal p_cur = currentPressure();
        if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5)) {
            CameraProcessor::setCapRate(1);
        }
        CameraProcessor::restoreDefaultCapRate();
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

    int bad_dp_count = 0;

    while (currentPressure() < p_target) {    
        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }

        const qreal p_cur = currentPressure();
        const qreal dp_cur = getCurrentPressureVelocity();
        m_dP_target = getMaxPressureVelocity();
        int freq = calculateFrequency(f_max, p_cur, p_target);

        if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5)) {
            CameraProcessor::setCapRate(1); // ускоряем захват кадров при подходе к узлам
            m_dP_target = getMinPressureVelocity();
            freq = freq / 3;
        }

        if (dp_cur < m_dP_target / 10) {
            bad_dp_count += 1;
        }
        else {
            if (bad_dp_count > 0)
                bad_dp_count -= 1;
        }

        if (bad_dp_count >= 20) {
            g540Driver()->setFrequency(0);
            int resp = requestUserConfirmation();
            // Если пользователь выбрал вариант, что двигатель не работает, то выходим из градуировки
            if (resp == USER_RESPONSE_FALSE) {
                interrupt;
            }
            // Иначе считаем что с двигателем всё в порядки и давление не поднимается из-за сильной утечки
            else {
                bad_dp_count = 0;
            }
        }

        CameraProcessor::restoreDefaultCapRate();
        g540Driver()->setFrequency(freq);
        QThread::msleep(90);
    }
}

void PressureControllerStand4::backwardPressure() {
    // Движение поршня назад до концевика
    const qreal p_target = getTargetPressure();
    const int f_max = g540Driver()->maxFrequency();
    g540Driver()->setDirection(G540Direction::Backward);

    // TODO: заменить на проверку срабатывания концевика, т. к. я для проверки это сделал
    // while (!isStartLimitTriggered()) {
    while (currentPressure() > 10.0) {
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
                CameraProcessor::setCapRate(1); // ускоряем захват кадров при подходе к узлам
                m_dP_target = getMinPressureVelocity();
                freq = freq / 3;
            }
            CameraProcessor::restoreDefaultCapRate();
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
    gs->graduator().switchToForward();
    preloadPressure(); // 1
    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    forwardPressure(); // 2
    gs->graduator().switchToBackward();
    gs->requestTableUpdate();
    backwardPressure(); // 3
    g540Driver()->stop();
    gs->requestTableUpdate();
    m_dP_target = 0;
    m_isRunning = false;
    emit successfullyStopped();
}

