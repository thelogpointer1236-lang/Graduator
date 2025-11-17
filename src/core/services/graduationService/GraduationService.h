#ifndef GRADUATOR_GRADUATIONSERVICE_H
#define GRADUATOR_GRADUATIONSERVICE_H
#include <QObject>
#include <QElapsedTimer>
#include <atomic>
#include "Graduator.h"
#include "core/types/GaugeModel.h"
#include "core/types/Pressure.h"
#include "core/types/PartyResult.h"
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
    PartyResult currentResult() const;
    bool hasResult() const;
    void setResult(const PartyResult &result);
signals:
    void currentResultChanged(bool hasResult);
private slots:
    void onPressureMeasured(qreal t, Pressure p);
    void onAngleMeasured(qint32 i, qreal t, qreal a);
    void onSuccessfullyStopped();
    void onInterrupted();
private:
    void pushPressure(qreal t, qreal p);
    void pushAngle(qint32 i, qreal t, qreal a);
    void connectObjects();
    void disconnectObjects();
    void clearCurrentResult();
    void emitResultChangedIfNeeded();
private:
    std::atomic<bool> m_isRunning = false;
    QElapsedTimer m_elapsedTimer;
    Graduator m_forwardGraduator;
    Graduator m_backwardGraduator;
    mutable GaugeModel m_gaugeModel;
    mutable PressureUnit m_pressureUnit = PressureUnit::Kgf;
    std::atomic<bool> m_isNearToPressurePoint = false;
    std::atomic<bool> m_forward = true;
    PartyResult m_currentResult;
    bool m_lastResultState = false;
};
#endif //GRADUATOR_GRADUATIONSERVICE_H
