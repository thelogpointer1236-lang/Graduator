#ifndef GRADUATOR_PRESSURECONTROLLERSTAND4_H
#define GRADUATOR_PRESSURECONTROLLERSTAND4_H

#include "../PressureControllerBase.h"

class PressureControllerStand4 final : public PressureControllerBase {
    Q_OBJECT
public:
    explicit PressureControllerStand4(QObject *parent = nullptr);
    ~PressureControllerStand4() override;

    void setMode(int modeIdx) override;
    QStringList getModes() const override;

    qreal getTargetPressure() const override;
    qreal getTargetPressureVelocity() const override;

    bool isReadyToStart(QString &err) const override;
    void start() override;

protected:
    void onPressureUpdated(qreal time, qreal pressure) override;

private:
    // Основные шаги
    void preloadPressure();
    void forwardPressure();
    void backwardPressure();

    // Вспомогательные вычисления
    qreal getPreloadPressure() const;
    qreal getNominalPressureVelocity() const;
    qreal getMaxPressureVelocity() const;
    qreal getMinPressureVelocity() const;

    // Логика узлов и гистерезиса
    bool isInThreshold(double p, double node, double percent) const;
    bool updateSlowModeByNode(double p_cur);

    // Логика управляющих частотой
    int computeFrequency(double p_cur, double p_target, int f_max);
    int computeDynamicDivider(double p_cur, double dp_cur) const;

    // Вспомогательные рутинные функции
    void applyCameraRateIfNearNode(double p_cur);
    bool handleBadVelocity(int &bad_dp_count, double dp_cur);
    void updateFlapsForBackward(double p_cur);

private:
    int   m_currentMode      = 0;
    qreal m_dP_target        = 0.0;
    bool  m_nearNodeSlowMode = false;  // гистерезисный режим «медленно»
};

#endif // GRADUATOR_PRESSURECONTROLLERSTAND4_H
