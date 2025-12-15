#include "PressureControllerStand4.h"

#include <QApplication>
#include <QThread>
#include <QDebug>
#include "core/services/ServiceLocator.h"
#include "utils.h"

#define MODE_INFERENCE               0
#define MODE_FORWARD                 1
#define MODE_FORWARD_AND_BACKWARD    2


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

qreal PressureControllerStand4::preloadFactor() const {
    qreal back = gaugePressureValues().back();
    if (qFuzzyCompare(back, 600)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.26;
    }
    if (qFuzzyCompare(back, 60)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.26;
    }

    if (qFuzzyCompare(back, 400)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.26;
    }
    if (qFuzzyCompare(back, 40)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.26;
    }

    if (qFuzzyCompare(back, 250)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.35;
    }
    if (qFuzzyCompare(back, 25)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.35;
    }

    if (qFuzzyCompare(back, 160)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 16)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.87;
    }

    if (qFuzzyCompare(back, 100)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 10)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.87;
    }

    if (qFuzzyCompare(back, 60)) {
        if (pressureUnit() == PressureUnit::Kgf || pressureUnit() == PressureUnit::Atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 6)) {
        if (pressureUnit() == PressureUnit::MPa)
            return 0.87;
    }

    return 0.0;
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
// Логика расчёта частоты
// ---------------------------------------------------------------------------

void PressureControllerStand4::updateFreq(double pCur, double pTarget,
                                          double dpCur, double dpTarget,
                                          double divMin, double divMax,
                                          double freqMin, double freqMax)
{
    if (qFuzzyIsNull(pTarget)) {
        m_frequency = freqMin;
        return;
    }

    constexpr double kUpperDeadZone = 1.7;
    constexpr double kLowerDeadZone = 0.7;
    constexpr double kDividerStepPercent = 1.0;

    const double absDpCur    = qAbs(dpCur);
    const double absDpTarget = qAbs(dpTarget);
    const double dividerStep = (divMax - divMin) * kDividerStepPercent / 100.0;

    if (absDpCur > kUpperDeadZone * absDpTarget) {
        m_divider += dividerStep;
    }
    else if (absDpCur < kLowerDeadZone * absDpTarget) {
        m_divider -= dividerStep;
    }

    m_divider = qBound(divMin, m_divider, divMax);

    const double freqBase =
        (freqMax - freqMin) * (pCur / pTarget) + freqMin;

    m_frequency = qBound(freqMin, freqBase / m_divider, freqMax);
}


// ---------------------------------------------------------------------------
// Прочие вспомогательные функции
// ---------------------------------------------------------------------------

void PressureControllerStand4::applyCameraRateIfNearNode(double p_cur)
{
    if (m_nearToPressureNode) {
        CameraProcessor::setCapRate(1);
    } else {
        CameraProcessor::restoreDefaultCapRate();
    }
}

bool PressureControllerStand4::handleBadVelocity(int &bad_dp_count, double dp_cur)
{
    // if (currentPressure() > gaugePressureValues().back() / 2
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

void PressureControllerStand4::updateNearingToPressureNode(double thr_l, double thr_r) {
    m_nearToPressureNode = isNearToPressureNode(currentPressure(), thr_l, thr_r, gaugePressureValues(), &m_nearingToPressureNode);
}

// ---------------------------------------------------------------------------
// Преднагрузка
// ---------------------------------------------------------------------------

bool PressureControllerStand4::preloadPressure()
{
    const qreal p_preload = getPreloadPressure();

    if (currentPressure() < p_preload && currentPressure() < gaugePressureValues()[1]) {
        g540Driver()->setFlapsState(G540FlapsState::OpenInput);
    }
    else {
        return false;
    }


    while (currentPressure() < p_preload) {

        applyCameraRateIfNearNode(currentPressure());

        if (shouldStop()) {
            g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
            return false;
        }

        QThread::msleep(15);
    }

    g540Driver()->setFlapsState(G540FlapsState::CloseBoth);
    return true;
}



// ---------------------------------------------------------------------------
// Прямой ход
// --------------------------------------------------------------------------

bool PressureControllerStand4::forwardPressure()
{
    const qreal p_target = getTargetPressure();
    const int f_max      = g540Driver()->maxFrequency();
    const int f_min      = g540Driver()->minFrequency();

    g540Driver()->setDirection(G540Direction::Forward);

    int bad_dp_count = 0;

    while (currentPressure() < p_target) {

        if (shouldStop()) {
            return false;
        }

        const double p_cur = currentPressure();
        const double dp_cur = getCurrentPressureVelocity();
        double dp_target = getNominalPressureVelocity() * 4;


        updateNearingToPressureNode(7.5, 7.5);

        if (m_currentMode == MODE_INFERENCE) {
            m_frequency = f_max;
        }
        else {
            double nearing;
            bool isNear = isNearToPressureNode(p_cur, 20.0, 10.0, gaugePressureValues(), &nearing);
            if (!isNear) {
                updateFreq(p_cur, p_target, dp_cur, dp_target, 0.1, 3.0, f_min, f_max);
            }
            else {
                dp_target = lerp(dp_target, dp_target / 5.0, nearing);
                updateFreq(p_cur, p_target, dp_cur, dp_target / 5.0, 0.1, 3.0, f_min, f_max);
            }
        }

        m_dP_target = dp_target;

        if (!handleBadVelocity(bad_dp_count, dp_cur)) {
            return false;
        }

        g540Driver()->setFrequency(m_frequency);

        QThread::msleep(90);
    }
    return true;
}



// ---------------------------------------------------------------------------
// Обратный ход
// ---------------------------------------------------------------------------

bool PressureControllerStand4::backwardPressure()
{
    const int f_max      = g540Driver()->maxFrequency();

    m_dP_target = getNominalPressureVelocity() * 4.0;

    g540Driver()->setDirection(G540Direction::Backward);

    while (!isEndLimitTriggered()) {
        if (shouldStop()) {
            return false;
        }

        const double p_cur = currentPressure();

        applyCameraRateIfNearNode(p_cur);

        double nearing;
        bool isNear = isNearToPressureNode(p_cur, 15.0, 25.0, gaugePressureValues(), &nearing);

        if (isNear) {
            m_frequency = lerp(f_max, f_max / 8.0, nearing);
        }
        else {
            m_frequency = f_max;
        }

        g540Driver()->setFrequency(m_frequency);
        QThread::msleep(90);
    }

    const double p_cur = currentPressure();
    if (p_cur <= getPreloadPressure() && p_cur < gaugePressureValues()[1]) {
        g540Driver()->setFlapsState(G540FlapsState::OpenOutput);
    }

    return true;
}



// ---------------------------------------------------------------------------
// Общий запуск
// ---------------------------------------------------------------------------

void PressureControllerStand4::start()
{
    Q_ASSERT(QThread::currentThread() != qApp->thread());

    auto *gs = ServiceLocator::instance().graduationService();
    auto finalize = [this, gs](bool success) {
        g540Driver()->stop();
        CameraProcessor::restoreDefaultCapRate();
        m_dP_target = 0;
        m_isRunning = false;
        gs->updateResult();

        if (success)
            emit resultReady();
        else
            emit interrupted();
    };

    if (qFuzzyIsNull(preloadFactor())) {
        ServiceLocator::instance().logger()->error("preloadFactor is null");
        finalize(false);
    }


    m_isRunning = true;
    emit started();

    gs->graduator().switchToForward();

    if (!preloadPressure()) {
        finalize(false);
        return;
    }

    QMetaObject::invokeMethod(g540Driver(), "start", Qt::QueuedConnection);

    if (!forwardPressure()) {
        finalize(false);
        return;
    }

    gs->graduator().switchToBackward();
    gs->updateResult();

    if (!backwardPressure()) {
        finalize(false);
        return;
    }

    finalize(true);
}

