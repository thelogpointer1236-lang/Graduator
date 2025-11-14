#ifndef GRADUATOR_AUTOMATICWIDGET_H

#define GRADUATOR_AUTOMATICWIDGET_H

#include <QWidget>

class QComboBox;
class QPushButton;

class AutomaticWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AutomaticWidget(QWidget *parent = nullptr);
    ~AutomaticWidget() override;

private slots:
    void onCalibrationModeChanged(int index);
    void onStartClicked();
    void onStopClicked();

private:
    void setupUi();
    void connectSignals();

    QComboBox *m_calibrationModeComboBox = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
};

#endif // GRADUATOR_AUTOMATICWIDGET_H
