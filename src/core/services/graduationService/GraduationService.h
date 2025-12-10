#ifndef GRADUATOR_GRADUATIONSERVICE_H
#define GRADUATOR_GRADUATIONSERVICE_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

#include "Graduator.h"
#include "core/types/GaugeModel.h"
#include "core/types/Pressure.h"
#include "core/types/PartyResult.h"

class GraduationService final : public QObject {
    Q_OBJECT

public:
    // FSM — состояние сервиса
    enum class State {
        Idle,
        Prepared,
        Running,
        Finished,
        Interrupted
    };

    explicit GraduationService(QObject *parent = nullptr);
    ~GraduationService() override;

    // ---------------------------------------------------------
    // Public API — строгий, предсказуемый интерфейс
    // ---------------------------------------------------------

    // 1) Этап подготовки: загружает модель, проверяет конфиг, готовит контекст
    bool prepare(QString &err);

    // 2) Этап запуска: запускает алгоритм только после успешного prepare()
    bool start();

    // 3) Прерывание процесса
    void interrupt();

    // 4) Состояние
    qreal getElapsedTimeSeconds() const;
    bool isResultReady() const;

    // 5) Результат
    const PartyResult& getPartyResult() const;

    void setStrongNode(bool strong);
    bool strongNode() const { return m_strongNode; }

    // Для внешнего доступа к вычислителю
    grad::Graduator& graduator() { return m_graduator; }

    State state() const { return m_state; }

    void requestUpdateResultAndTable();

    // Обновление результата
    void updateResult();

    int angleMeasCountForCamera(qint32 idx, bool isForward) const;

signals:
    // События жизненного цикла
    void started();
    void interrupted();
    void ended();

    // UI-сигналы
    void resultAvailabilityChanged(bool available);
    void tableUpdateRequired();

private slots:
    // Обработчики данных в процессе градуировки
    void onPressureMeasured(qreal t, Pressure p);
    void onAngleMeasured(qint32 idx, qreal t, qreal angle);

    void checkSensorsActivity();

    // События контроллера давления
    void onPressureControllerResultReady();
    void onPressureControllerInterrupted();

private:

    // Управление состояниями
    void connectObjects();
    void disconnectObjects();

    // Очистка при новом запуске
    void clearForNewRun();
    void clearResultOnly();

    void stopWatchdog();



private:
    // Текущее состояние
    State m_state = State::Idle;

    // Таймер длительности градуировки
    QElapsedTimer m_elapsedTimer;

    // Контроль активности датчиков
    QTimer m_watchdogTimer;
    qreal m_lastPressureTimestamp = 0.0;
    qreal m_lastAngleTimestamp = 0.0;

    // Основной вычислитель
    grad::Graduator m_graduator;

    // Модель прибора и единица давления
    GaugeModel   m_gaugeModel;
    PressureUnit m_pressureUnit = PressureUnit::Unknown;

    // Готовый результат
    PartyResult m_currentResult;
    bool m_resultReady = false;

    bool m_strongNode = false;
}; 

#endif // GRADUATOR_GRADUATIONSERVICE_H
