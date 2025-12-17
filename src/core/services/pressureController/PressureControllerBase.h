#ifndef GRADUATOR_PRESSURECONTROLLER_H
#define GRADUATOR_PRESSURECONTROLLER_H
#include <QObject>
#include <QPointer>
#include <QThread>
#include "core/types/GaugeModel.h"
#include "core/types/Pressure.h"
#include "drivers/G540Driver.h"

enum class PressureControllerMode {
    Inference,
    Forward,
    ForwardBackward
};


class PressureControllerBase : public QObject {
    Q_OBJECT
signals:
    void interrupted();
    void resultReady();
    void started();

public:
    explicit PressureControllerBase(QObject *parent = nullptr);
    ~PressureControllerBase() override;
    G540Driver *g540Driver() const;
    void setGaugePressurePoints(const std::vector<qreal> &pressureValues);
    void setPressureUnit(PressureUnit unit);
    virtual qreal preloadFactor() const = 0;
    void updatePressure(qreal time, qreal pressure);
    virtual qreal getTargetPressure() const = 0;
    virtual qreal getTargetPressureVelocity() const = 0;
    quint32 getImpulsesCount() const;
    quint32 getImpulsesFreq() const;
    qreal getCurrentPressureVelocity() const;
    qreal getCurrentPressure() const;
    qreal getTimeSinceStartSec() const;
    bool isStartLimitTriggered() const;
    bool isEndLimitTriggered() const;
    bool isAnyLimitTriggered() const;
    void openInletFlap();
    void openOutletFlap();
    void closeBothFlaps();
    virtual bool isReadyToStart(QString &err) const;
    bool isRunning() const;

    void setMode(PressureControllerMode mode);
    PressureControllerMode getMode() const;

public slots:
    Q_INVOKABLE virtual void startGoToEnd();
    Q_INVOKABLE virtual void startGoToStart();
    Q_INVOKABLE virtual void start() = 0;
    Q_INVOKABLE void interrupt();

protected:
    virtual void onPressureUpdated(qreal time, qreal pressure) = 0;
    const std::vector<qreal> &gaugePressureValues() const;
    PressureUnit pressureUnit() const;

    qreal currentPressure() const;
    bool shouldStop() const noexcept;

private:
    std::unique_ptr<G540Driver> m_G540Driver;
    QThread *m_G540DriverThread = nullptr;
    std::vector<qreal> m_gaugePressureValues;
    PressureUnit m_pressureUnit{};
    std::atomic<bool> m_aboutToStop{false};

protected:
    std::atomic<bool> m_isRunning{false};
    std::vector<qreal> m_pressure_history;
    std::vector<qreal> m_time_history;
    std::vector<int> m_c_imp_history;
    std::vector<int> m_freq_history;
    PressureControllerMode m_mode = PressureControllerMode::Inference;
};
#endif //GRADUATOR_PRESSURECONTROLLER_H
