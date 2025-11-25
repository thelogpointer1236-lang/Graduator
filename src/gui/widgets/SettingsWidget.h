#ifndef GRADUATOR_SETTINGSWIDGET_H
#define GRADUATOR_SETTINGSWIDGET_H

#include <QWidget>
#include <QColor>

class QGroupBox;
class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QVBoxLayout;
class QSlider;
class QLabel;

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

    // Новые слоты
    void onAimRadiusChanged(int value);
    void onAimColorPickRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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

private:
    // Cameras group
    QLineEdit *editCameras_ = nullptr;
    QCheckBox *chkAutoOpen_ = nullptr;
    QPushButton *btnOpen_ = nullptr;
    QPushButton *btnOpenAll_ = nullptr;

    // Gauge settings
    QComboBox *comboDeviceType_ = nullptr;
    QComboBox *comboUnit_ = nullptr;
    QComboBox *comboAccuracy_ = nullptr;

    // Printing settings
    QComboBox *comboPrinter_ = nullptr;
    QComboBox *comboDisplacement_ = nullptr;

    // Pressure sensor
    QLineEdit *editComPort_ = nullptr;
    QPushButton *btnConnectCom_ = nullptr;
    QCheckBox *chkDrawCrosshair_ = nullptr;

    // NEW: Crosshair radius
    QSlider *sliderAimRadius_ = nullptr;
    QLabel *lblAimRadiusValue_ = nullptr;

    // NEW: Color picker
    QPushButton *btnAimColor_ = nullptr;
    QLabel *colorPreview_ = nullptr;
    QColor currentAimColor_;
};

#endif // GRADUATOR_SETTINGSWIDGET_H
