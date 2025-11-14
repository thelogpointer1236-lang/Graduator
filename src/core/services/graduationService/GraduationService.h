#ifndef GRADUATOR_GRADUATIONSERVICE_H
#define GRADUATOR_GRADUATIONSERVICE_H
#include <QObject>
#include <QElapsedTimer>
#include <atomic>
#include "Graduator.h"
#include "core/types/GaugeModel.h"
#include "core/types/Pressure.h"
class GraduationService final : public QObject {
    Q_OBJECT
public:
    explicit GraduationService(QObject *parent = nullptr);
    ~GraduationService() override;
    void start();
    void stop();
    bool isRunning() const;
    bool isReadyToRun(QString &err) const;
    std::vector<std::vector<double> > graduateForward();
    std::vector<std::vector<double> > graduateBackward();
    void switchToBackward();
    void switchToForward();
    qreal getElapsedTimeSeconds() const;
private slots:
    void onPressureMeasured(qreal t, Pressure p);
    void onAngleMeasured(qint32 i, qreal t, qreal a);
private:
    void pushPressure(qreal t, qreal p);
    void pushAngle(qint32 i, qreal t, qreal a);
    void connectObjects();
    void disconnectObjects();
private:
    std::atomic<bool> m_isRunning = false;
    QElapsedTimer m_elapsedTimer;
    Graduator m_forwardGraduator;
    Graduator m_backwardGraduator;
    mutable GaugeModel m_gaugeModel;
    mutable PressureUnit m_pressureUnit = PressureUnit::Kgf;
    std::atomic<bool> m_recording = false;
    std::atomic<bool> m_forward = true;
};
#endif //GRADUATOR_GRADUATIONSERVICE_H
