#ifndef GRADUATOR_SETTINGSWIDGET_H

#define GRADUATOR_SETTINGSWIDGET_H

#include <QWidget>

class QGroupBox;
class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QVBoxLayout;

class SettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);


private slots:
    void onOpenCameraClicked();
    void onOpenAllCamerasClicked();
    void onAutoOpenCamerasChanged(bool checked);
    void onGaugeTypeChanged(int index);
    void onPressureUnitChanged(int index);
    void onPrecisionClassChanged(int index);
    void onPrinterChanged(int index);
    void onDialLayoutChanged(int index);
    void onConnectComPortClicked();
    void onDrawCrosshairChanged(bool checked);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void installEventFilters();
    void setupUi();
    QVBoxLayout *createMainLayout();
    QGroupBox *createCamerasGroup();
    QGroupBox *createDeviceGroup();
    QGroupBox *createPrintGroup();
    QGroupBox *createMiscGroup();
    void applyStyles();
    void connectSignals();

    QLineEdit *editCameras_ = nullptr;
    QCheckBox *chkAutoOpen_ = nullptr;
    QPushButton *btnOpen_ = nullptr;
    QPushButton *btnOpenAll_ = nullptr;
    QComboBox *comboDeviceType_ = nullptr;
    QComboBox *comboUnit_ = nullptr;
    QComboBox *comboAccuracy_ = nullptr;
    QComboBox *comboPrinter_ = nullptr;
    QComboBox *comboDialLayout_ = nullptr;
    QLineEdit *editComPort_ = nullptr;
    QPushButton *btnConnectCom_ = nullptr;
    QCheckBox *chkDrawCrosshair_ = nullptr;
};

#endif // GRADUATOR_SETTINGSWIDGET_H
