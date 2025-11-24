#ifndef GRADUATOR_MANUALCONTROLWIDGET_H

#define GRADUATOR_MANUALCONTROLWIDGET_H

#include <QWidget>

class QPushButton;
class QRadioButton;
class QGridLayout;
class QGroupBox;

class ManualWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ManualWidget(QWidget *parent = nullptr);
    ~ManualWidget() override;

private slots:
    void onStartMotor();
    void onStopMotor();
    void onOpenInlet() const;
    void onOpenOutlet() const;
    void onCloseBoth() const;
    void updateUiState();      // Слот для централизованного обновления UI
    void onPollTimerTimeout(); // Слот, который будет вызываться таймером

private:
    void setupUi();
    QGroupBox *createDirectionGroup();
    QGroupBox *createMotorGroup();
    QGroupBox *createValveGroup();
    void connectSignals();

    QRadioButton *m_radioForward = nullptr;
    QRadioButton *m_radioBackward = nullptr;
    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnStopMotor = nullptr;
    QPushButton *m_btnOpenInlet = nullptr;
    QPushButton *m_btnOpenOutlet = nullptr;
    QPushButton *m_btnCloseBoth = nullptr;

    QTimer* m_pollTimer = nullptr;
};

#endif // GRADUATOR_MANUALCONTROLWIDGET_H
