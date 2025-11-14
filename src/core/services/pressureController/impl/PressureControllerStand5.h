#ifndef GRADUATOR_PRESSURECONTROLLERSTAND5_H
#define GRADUATOR_PRESSURECONTROLLERSTAND5_H
#include "../PressureControllerBase.h"
class PressureControllerStand5 final : public PressureControllerBase {
    Q_OBJECT
public:
    explicit PressureControllerStand5(QObject *parent = nullptr);
    ~PressureControllerStand5() override;
    void setMode(int modeIdx) override;
    QStringList getModes() const override;
    qreal getTargetPressure() const override;
    qreal getTargetPressureVelocity() const override;
    void start() override;
    bool isReadyToStart(QString &err) const override;
protected:
    void onPressureUpdated(qreal time, qreal pressure) override;
private:
    void preloadPressure();
    void forwardPressure();
    void backwardPressure();
    qreal getPreloadPressure() const;
};
#endif //GRADUATOR_PRESSURECONTROLLERSTAND5_H