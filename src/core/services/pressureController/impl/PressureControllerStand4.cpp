#include "PressureControllerStand4.h"

#include <QThread>
#include <QDebug>
#include "core/services/ServiceLocator.h"
#include "utils.h"

#define interrupt \
    {m_isRunning = false; \
    CameraProcessor::restoreDefaultCapRate(); \
    emit interrupted(); \
    return;}

#define MODE_INFERENCE               0
#define MODE_FORWARD                 1
#define MODE_FORWARD_AND_BACKWARD    2

namespace {

double calculateFrequency(double maxFrequency,
                          double currentPressure,
                          double limitPressure)
{
    if (currentPressure >= limitPressure)
        return maxFrequency / 4.0;

    if (currentPressure <= 0)
        return maxFrequency;

    double ratio = currentPressure / limitPressure;
    double delta = (maxFrequency * 3.0 / 4.0) * ratio;

    return maxFrequency - delta;
}

bool isNearToPressureNode(const std::vector<double> &nodes,
                          double p,
                          double percentThreshold)
{
    if (nodes.size() < 2)
        return false;

    double step = nodes[1] - nodes[0];
    double threshold = std::abs(step * percentThreshold / 100.0);

    for (double node : nodes)
        if (std::abs(p - node) <= threshold)
            return true;

    return false;
}

} // namespace



PressureControllerStand4::PressureControllerStand4(QObject *parent)
    : PressureControllerBase(parent)
{
}

PressureControllerStand4::~PressureControllerStand4() = default;



void PressureControllerStand4::setMode(int modeIdx)
{
    m_currentMode = modeIdx;
}

QStringList PressureControllerStand4::getModes() const
{
    return {
        QString::fromWCharArray(L"Прицел"),
        QString::fromWCharArray(L"Прямой ход"),
        QString::fromWCharArray(L"Прямой и обратный ход")
    };
}



qreal PressureControllerStand4::getTargetPressure() const
{
    if (gaugePressureValues().size() < 2)
        return 0.0;

    return gaugePressureValues().back() * 1.01;
}

qreal PressureControllerStand4::getTargetPressureVelocity() const
{
    return m_dP_target;
}



bool PressureControllerStand4::isReadyToStart(QString &err) const
{
    if (!PressureControllerBase::isReadyToStart(err))
        return false;

    if (gaugePressureValues().size() < 2) {
        err = QString::fromWCharArray(
            L"Недостаточно точек шкалы давления для работы контроллера."
        );
        return false;
    }

    if (currentPressure() > getPreloadPressure()) {
        err = QString::fromWCharArray(
            L"Текущее давление превысило давление преднагрузки."
        );
        return false;
    }

    return true;
}



qreal PressureControllerStand4::getPreloadPressure() const
{
    if (gaugePressureValues().size() < 2)
        return 0.0;

    return gaugePressureValues()[1] * preloadFactor();
}

qreal PressureControllerStand4::getNominalPressureVelocity() const
{
    if (gaugePressureValues().size() < 2)
        return 0.0;

    return gaugePressureValues().back() / getNominalDurationSec();
}

qreal PressureControllerStand4::getMaxPressureVelocity() const
{
    return getNominalPressureVelocity() * getMaxVelocityFactor();
}

qreal PressureControllerStand4::getMinPressureVelocity() const
{
    return getNominalPressureVelocity() * getMinVelocityFactor();
}



void PressureControllerStand4::onPressureUpdated(qreal time, qreal pressure)
{
    Q_UNUSED(time)
    Q_UNUSED(pressure)
}



// ---------------------------------------------------------------------------
// Узловая логика с гистерезисом (7.5% вход, 3.5% выход)
// ---------------------------------------------------------------------------

bool PressureControllerStand4::isInThreshold(double p, double node, double percent) const
{
    double step = gaugePressureValues()[1] - gaugePressureValues()[0];
    double threshold = std::abs(step * percent / 100.0);
    return std::abs(p - node) <= threshold;
}

bool PressureControllerStand4::updateSlowModeByNode(double p_cur)
{
    const auto &nodes = gaugePressureValues();
    if (nodes.size() < 2) return false;

    for (double node : nodes) {

        // Вход в медленный режим — 7.5%
        if (!m_nearNodeSlowMode && isInThreshold(p_cur, node, 7.5)) {
            m_nearNodeSlowMode = true;
            return true;
        }

        // Выход из медленного режима — 3.5%
        if (m_nearNodeSlowMode && !isInThreshold(p_cur, node, 3.5)) {
            m_nearNodeSlowMode = false;
            return false;
        }
    }

    return m_nearNodeSlowMode;
}



// ---------------------------------------------------------------------------
// Логика расчёта частоты
// ---------------------------------------------------------------------------

int PressureControllerStand4::computeDynamicDivider(double p_cur, double dp_cur) const
{
    const double vmax = getMaxPressureVelocity();
    if (vmax <= 0)
        return 2;

    double speedRatio = std::clamp(dp_cur / vmax, 0.0, 1.0);
    double dynamicDivider = 1.0 + 3.0 * speedRatio; // плавный переход 1 → 4

    if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5))
        dynamicDivider *= 1.3;

    double pmax = gaugePressureValues().empty() ? 300 : gaugePressureValues().back();
    double scaleFactor = std::clamp(pmax / 400.0, 0.5, 1.5);
    dynamicDivider *= scaleFactor;

    int divider = static_cast<int>(std::round(dynamicDivider));
    return std::clamp(divider, 1, 5);
}

int PressureControllerStand4::computeFrequency(double p_cur, double p_target, int f_max)
{
    int freq = calculateFrequency(f_max, p_cur, p_target);

    // Гистерезисный контроль
    bool slowMode = updateSlowModeByNode(p_cur);

    if (slowMode) {
        int divider = computeDynamicDivider(p_cur, getCurrentPressureVelocity());
        freq = std::max(1, freq / divider);
        m_dP_target = getMinPressureVelocity();
    } else {
        m_dP_target = getMaxPressureVelocity();
    }

    return freq;
}



// ---------------------------------------------------------------------------
// Прочие вспомогательные функции
// ---------------------------------------------------------------------------

void PressureControllerStand4::applyCameraRateIfNearNode(double p_cur)
{
    if (isNearToPressureNode(gaugePressureValues(), p_cur, 7.5)) {
        CameraProcessor::setCapRate(1);
    } else {
        CameraProcessor::restoreDefaultCapRate();
    }
}

bool PressureControllerStand4::handleBadVelocity(int &bad_dp_count, double dp_cur)
{
    if (dp_cur < m_dP_target / 10)
        ++bad_dp_count;
    else if (bad_dp_count > 0)
        --bad_dp_count;

    if (bad_dp_count < 20)
        return true;

    g540Driver()->setFrequency(0);

    const QString USER_FALSE = tr("Roll the engine back");
    const QString USER_TRUE  = tr("The engine is fine");

    QString resp =
        ServiceLocator::instance().userDialogService()->requestUserInput(
            tr("The engine is stuck"),
            tr("The engine appears to be stuck. For safety reasons, the program has paused. Please check it and respond."),
            { USER_FALSE, USER_TRUE }
        );

    if (resp == USER_TRUE) {
        bad_dp_count = 0;
        return true;
    }

    return false;
}

void PressureControllerStand4::updateFlapsForBackward(double p_cur)
{
    if (p_cur <= getPreloadPressure())
        g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
    else
        g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
}



// ---------------------------------------------------------------------------
// Преднагрузка
// ---------------------------------------------------------------------------

void PressureControllerStand4::preloadPressure()
{
    const qreal p_preload = getPreloadPressure();

    if (currentPressure() < p_preload) {
        g540Driver()->setFlapsState(G540FlapsState::OpenInput);
    }
    else {
        interrupt;
    }


    while (currentPressure() < p_preload) {

        applyCameraRateIfNearNode(currentPressure());

        if (shouldStop()) {
            g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
            interrupt;
        }

        QThread::msleep(15);
    }

    g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
}



// ---------------------------------------------------------------------------
// Прямой ход
// ---------------------------------------------------------------------------

void PressureControllerStand4::forwardPressure()
{
    const qreal p_target = getTargetPressure();
    const int f_max      = g540Driver()->maxFrequency();

    g540Driver()->setDirection(G540Direction::Forward);

    int bad_dp_count = 0;

    while (currentPressure() < p_target) {

        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }

        const double p_cur = currentPressure();
        const double dp_cur = getCurrentPressureVelocity();

        int freq;
        if (m_currentMode == MODE_INFERENCE) {
            freq = f_max;
        }
        else {
            freq = computeFrequency(p_cur, p_target, f_max);
        }

        if (!handleBadVelocity(bad_dp_count, dp_cur))
            interrupt;

        g540Driver()->setFrequency(freq);

        QThread::msleep(90);
    }
}



// ---------------------------------------------------------------------------
// Обратный ход
// ---------------------------------------------------------------------------

void PressureControllerStand4::backwardPressure()
{
    const qreal p_target = getTargetPressure();
    const int f_max      = g540Driver()->maxFrequency();

    g540Driver()->setDirection(G540Direction::Backward);

    while (currentPressure() > 10.0) {

        if (shouldStop()) {
            g540Driver()->stop();
            interrupt;
        }

        const double p_cur = currentPressure();

        if (m_currentMode == MODE_FORWARD_AND_BACKWARD) {

            applyCameraRateIfNearNode(p_cur);
            updateFlapsForBackward(p_cur);

            int freq = computeFrequency(p_cur, p_target, f_max);
            g540Driver()->setFrequency(freq);
        }
        else {
            g540Driver()->setFrequency(f_max);
        }

        QThread::msleep(90);
    }

    updateFlapsForBackward(currentPressure());
}



// ---------------------------------------------------------------------------
// Общий запуск
// ---------------------------------------------------------------------------

void PressureControllerStand4::start()
{
    m_isRunning = true;
    emit started();

    auto *gs = ServiceLocator::instance().graduationService();

    gs->graduator().switchToForward();
    preloadPressure();

    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);
    forwardPressure();

    gs->graduator().switchToBackward();
    gs->updateResult();

    backwardPressure();

    g540Driver()->stop();
    gs->updateResult();

    m_dP_target = 0;
    m_isRunning = false;

    emit resultReady();
}
