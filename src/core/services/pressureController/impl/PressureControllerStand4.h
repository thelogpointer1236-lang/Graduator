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
    void preloadPressure();
    void forwardPressure();
    void backwardPressure();
    qreal getPreloadPressure() const;
    qreal getNominalPressureVelocity() const;
    qreal getMaxPressureVelocity() const;
    qreal getMinPressureVelocity() const;

private:
    int m_currentMode = 0;
    qreal m_dP_target = 0.0;
};
#endif //GRADUATOR_PRESSURECONTROLLERSTAND4_H
