#ifndef GRADUATOR_GRADUATIONSERVICE_H
#define GRADUATOR_GRADUATIONSERVICE_H
#include <QObject>
#include <QElapsedTimer>

#include "Graduator.h"
#include "core/types/GaugeModel.h"
#include "core/types/Pressure.h"
#include "core/types/PartyResult.h"

class GraduationService final : public QObject {
    Q_OBJECT
public:
    explicit GraduationService(QObject *parent = nullptr);
    ~GraduationService() override;

    qreal getElapsedTimeSeconds() const;

    void start();
    void interrupt();
    bool isRunning() const;
    bool isReadyToRun(QString &err) const;

    grad::Graduator& graduator();
    bool isResultReady() const;
    bool isResultSaved() const;
    const PartyResult& getPartyResult();
    void markResultSaved();
    void requestTableUpdate();

signals:
    void started();
    void interrupted();
    void successfullyStopped();
    void resultAvailabilityChanged(bool available);
    void tableUpdateRequired();

private slots:
    void onPressureMeasured(qreal t, Pressure p);
    void onAngleMeasured(qint32 i, qreal t, qreal a);
    void onControllerSuccessfullyStopped();
    void onControllerInterrupted();

private:
    void updateResult();

    void connectObjects();
    void disconnectObjects();

    void clearCurrentResult();

private:
    bool m_isRunning = false;
    QElapsedTimer m_elapsedTimer;

    grad::Graduator m_graduator;
    mutable GaugeModel m_gaugeModel;
    mutable PressureUnit m_pressureUnit = PressureUnit::Kgf;

    PartyResult m_currentResult;
    bool m_resultReady = false;
    bool m_resultSaved = true;
};
#endif //GRADUATOR_GRADUATIONSERVICE_H
