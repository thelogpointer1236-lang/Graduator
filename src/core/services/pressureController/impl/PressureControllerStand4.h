#ifndef GRADUATOR_PRESSURECONTROLLERSTAND4_H
#define GRADUATOR_PRESSURECONTROLLERSTAND4_H

#include "../PressureControllerBase.h"

class PressureControllerStand4 final : public PressureControllerBase {
    Q_OBJECT
public:
    explicit PressureControllerStand4(QObject *parent = nullptr);
    ~PressureControllerStand4() override;

    qreal getTargetPressure() const override;
    qreal getTargetPressureVelocity() const override;

    bool isReadyToStart(QString &err) const override;
    void start() override;

   qreal preloadFactor() const override;

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

    void updateFreq(double p_cur, double p_target,
                    double dp_cur, double dp_target,
                    double div_min, double div_max,
                    double freq_min, double freq_max
                    );

    // Вспомогательные рутинные функции
    void applyCameraRateIfNearNode(double p_cur);
    void updateNearingToPressureNode(double thr_l, double thr_r);

private:

    qreal m_dP_target        = 0.0;

    bool  m_nearToPressureNode = false;
    double m_nearingToPressureNode = 0.0;

    mutable std::vector<qreal> m_gaugePressureValuesInference;

    double m_frequency;
    double m_divider = 0;

    QElapsedTimer m_updateTimer;
    qint64 m_lastNs = 0;

    double m_overMs  = 0.0;   // сколько времени подряд "слишком большой dp"
    double m_underMs = 0.0;   // сколько времени подряд "слишком маленький dp"
};

#endif // GRADUATOR_PRESSURECONTROLLERSTAND4_H
