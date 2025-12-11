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
    bool preloadPressure();
    bool forwardPressure();
    bool backwardPressure();



    // Вспомогательные вычисления
    qreal getPreloadPressure() const;
    qreal getNominalPressureVelocity() const;
    qreal getMaxPressureVelocity() const;
    qreal getMinPressureVelocity() const;
    double m_frequency;
    double m_divider = 1;
    void updateFreq(double p_cur, double p_target,
                    double dp_cur, double dp_target,
                    double div_min, double div_max,
                    double freq_min, double freq_max
                    );
    // Логика узлов и гистерезиса
    bool isPressureNearToNode(double left, double right);

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
