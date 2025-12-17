#include "PressureControllerStand4.h"

#include <QApplication>
#include <QThread>
#include <QDebug>
#include "core/services/ServiceLocator.h"
#include "utils.h"

PressureControllerStand4::PressureControllerStand4(QObject *parent)
    : PressureControllerBase(parent)
{
}

PressureControllerStand4::~PressureControllerStand4() = default;


qreal PressureControllerStand4::preloadFactor() const {
    qreal back = gaugePressureValues().back();
    if (qFuzzyCompare(back, 600)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.26;
    }
    if (qFuzzyCompare(back, 60)) {
        if (pressureUnit() == PressureUnit::mpa)
            return 0.26;
    }

    if (qFuzzyCompare(back, 400)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.26;
    }
    if (qFuzzyCompare(back, 40)) {
        if (pressureUnit() == PressureUnit::mpa)
            return 0.26;
    }

    if (qFuzzyCompare(back, 250)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.35;
    }
    if (qFuzzyCompare(back, 25)) {
        if (pressureUnit() == PressureUnit::mpa)
            return 0.35;
    }

    if (qFuzzyCompare(back, 160)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 16)) {
        if (pressureUnit() == PressureUnit::mpa)
            return 0.87;
    }

    if (qFuzzyCompare(back, 100)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 10)) {
        if (pressureUnit() == PressureUnit::mpa)
            return 0.87;
    }

    if (qFuzzyCompare(back, 60)) {
        if (pressureUnit() == PressureUnit::kgf || pressureUnit() == PressureUnit::atm)
            return 0.87;
    }
    if (qFuzzyCompare(back, 6)) {
        if (pressureUnit() == PressureUnit::mpa)
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

    while (currentPressure() < p_target) {

        if (shouldStop()) {
            return false;
        }

        const double p_cur = currentPressure();
        const double dp_cur = getCurrentPressureVelocity();
        double dp_target = getNominalPressureVelocity() * 1.0;


        updateNearingToPressureNode(7.5, 7.5);
        applyCameraRateIfNearNode(p_cur);

        if (m_mode == PressureControllerMode::Inference) {
            updateFreq(p_cur, p_target, dp_cur, dp_target * 1.75, 0.1, 10.0, f_min, f_max);
        }
        else {
            updateFreq(p_cur, p_target, dp_cur, dp_target, 0.1, 10.0, f_min, f_max);
        }

        m_dP_target = dp_target;

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
    const qreal p_preload = getPreloadPressure();

    m_dP_target = getNominalPressureVelocity() * 2.0;

    g540Driver()->setDirection(G540Direction::Backward);

#ifdef USE_STUB_IMPLEMENTATIONS
    while (currentPressure() > 10) {
#else
    while (!isStartLimitTriggered()) {
#endif
        if (shouldStop()) {
            return false;
        }

        const double p_cur = currentPressure();

        updateNearingToPressureNode(7.5, 7.5);
        applyCameraRateIfNearNode(p_cur);

        if (p_cur < p_preload || m_mode == PressureControllerMode::Inference) {
            m_frequency = f_max;
        }
        else {
            double nearing;
            bool isNear = isNearToPressureNode(p_cur, 25.0, 35.0, gaugePressureValues(), &nearing);

            if (isNear) {
                m_frequency = lerp(f_max / 2.0, f_max / 20.0, nearing);
            }
            else {
                m_frequency = f_max / 3.0;
            }
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

