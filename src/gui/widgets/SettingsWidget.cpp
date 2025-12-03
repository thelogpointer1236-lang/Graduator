#include "SettingsWidget.h"

#include "core/services/ServiceLocator.h"
#include "defines.h"

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QEvent>
#include <QColorDialog>
#include <QSlider>
#include <QFrame>
#include <vector>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    connectSignals();
}

void SettingsWidget::setupUi()
{
    auto *mainLayout = createMainLayout();
    mainLayout->addWidget(createCamerasGroup());
    mainLayout->addWidget(createDeviceGroup());
    mainLayout->addWidget(createPrintGroup());
    mainLayout->addWidget(createMiscGroup());
    mainLayout->addStretch();
    setLayout(mainLayout);
    installEventFilters();
    applyStyles();
}

QVBoxLayout *SettingsWidget::createMainLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);
    return layout;
}

QGroupBox *SettingsWidget::createCamerasGroup()
{
    auto *group = new QGroupBox(tr("Cameras"), this);
    auto *layout = new QGridLayout(group);

    const QString camStr = ServiceLocator::instance().cameraProcessor()->cameraStr();
    auto *label = new QLabel(tr("Camera string") + ":", group);
    editCameras_ = new QLineEdit(camStr, group);
    btnOpen_ = new QPushButton(tr("Open"), group);
    btnOpenAll_ = new QPushButton(tr("Open all"), group);
    chkAutoOpen_ = new QCheckBox(tr("Automatically open all cameras when the application starts"), group);
    chkAutoOpen_->setChecked(ServiceLocator::instance().configManager()->get<bool>(CFG_KEY_SYS_AUTOOPEN, false));

    layout->addWidget(label, 0, 0);
    layout->addWidget(editCameras_, 0, 1);
    layout->addWidget(btnOpen_, 0, 2);
    layout->addWidget(btnOpenAll_, 1, 1);
    layout->addWidget(chkAutoOpen_, 2, 1);
    group->setLayout(layout);

    return group;
}

QGroupBox *SettingsWidget::createDeviceGroup()
{
    auto *group = new QGroupBox(tr("Gauge"), this);
    auto *layout = new QFormLayout(group);

    auto &locator = ServiceLocator::instance();
    auto *catalog = locator.gaugeCatalog();
    auto partyManager = locator.partyManager();


    comboDeviceType_ = new QComboBox(group);
    comboDeviceType_->addItems(catalog->allNames());
    comboDeviceType_->setCurrentIndex(locator.configManager()->get<int>(CFG_KEY_CURRENT_GAUGE_MODEL, 0));

    comboUnit_ = new QComboBox(group);
    comboUnit_->addItems(partyManager->getAvailablePressureUnits());
    comboUnit_->setCurrentIndex(locator.configManager()->get<int>(CFG_KEY_CURRENT_PRESSURE_UNIT, 0));

    comboAccuracy_ = new QComboBox(group);
    comboAccuracy_->addItems(partyManager->getAvailablePrecisions());
    comboAccuracy_->setCurrentIndex(locator.configManager()->get<int>(CFG_KEY_CURRENT_PRECISION_CLASS, 0));

    layout->addRow(tr("Gauge model") + ":", comboDeviceType_);
    layout->addRow(tr("Gauge pressure unit") + ":", comboUnit_);
    layout->addRow(tr("Gauge accuracy") + ":", comboAccuracy_);

    group->setLayout(layout);
    return group;
}

QGroupBox *SettingsWidget::createPrintGroup()
{
    auto *group = new QGroupBox(tr("Printing"), this);
    auto *layout = new QFormLayout(group);
    auto *config = ServiceLocator::instance().configManager();
    auto *partyManager = ServiceLocator::instance().partyManager();

    comboPrinter_ = new QComboBox(group);
    QStringList printers;
    for (const auto& x : partyManager->getAvailablePrinters()) {
        printers.append(x.name());
    }
    comboPrinter_->addItems(printers);
    comboPrinter_->setCurrentIndex(config->get<int>(CFG_KEY_CURRENT_PRINTER, 0));

    comboDisplacement_ = new QComboBox(group);
    QStringList displacements;
    for (const auto& x : partyManager->getAvailableDisplacements()) {
        displacements.append(x.name());
    }
    comboDisplacement_->addItems(displacements);
    comboDisplacement_->setCurrentIndex(config->get<int>(CFG_KEY_CURRENT_DIAL_LAYOUT, 0));

    layout->addRow(tr("Printer") + ":", comboPrinter_);
    layout->addRow(tr("Displacement") + ":", comboDisplacement_);

    group->setLayout(layout);
    return group;
}

QGroupBox *SettingsWidget::createMiscGroup()
{
    auto *group = new QGroupBox(tr("Pressure sensor"), this);
    auto *layout = new QGridLayout(group);

    auto *label = new QLabel(tr("Pressure sensor COM port") + ":", group);
    editComPort_ = new QLineEdit(group);
    editComPort_->setText(ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_PRESSURE_SENSOR_COM, "COM1"));
    btnConnectCom_ = new QPushButton(tr("Connect"), group);

    chkDrawCrosshair_ = new QCheckBox(tr("Draw a crosshair on the video output"), group);
    chkDrawCrosshair_->setChecked(ServiceLocator::instance().configManager()->get<bool>(CFG_KEY_AIM_VISIBLE, false));

    // -------------------------
    // НОВОЕ: СЛАЙДЕР РАДИУСА
    // -------------------------
    int radius = ServiceLocator::instance().configManager()->get<int>(CFG_KEY_AIM_RADIUS, 30);
    sliderAimRadius_ = new QSlider(Qt::Horizontal, group);
    sliderAimRadius_->setRange(5, 200);
    sliderAimRadius_->setValue(radius);

    lblAimRadiusValue_ = new QLabel(QString::number(radius), group);

    // -------------------------
    // НОВОЕ: COLOR PICKER
    // -------------------------
    btnAimColor_ = new QPushButton(tr("Choose color"), group);

    QString savedColor = ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_AIM_COLOR, "#FF0000");
    currentAimColor_ = QColor(savedColor);

    colorPreview_ = new QLabel(group);
    colorPreview_->setFixedSize(24, 24);
    colorPreview_->setFrameStyle(QFrame::Box | QFrame::Plain);
    colorPreview_->setStyleSheet(QString("background-color: %1").arg(savedColor));

    layout->addWidget(label, 0, 0);
    layout->addWidget(editComPort_, 0, 1);
    layout->addWidget(btnConnectCom_, 0, 2);
    layout->addWidget(chkDrawCrosshair_, 1, 1);

    layout->addWidget(new QLabel(tr("Crosshair radius") + ":", group), 2, 0);
    layout->addWidget(sliderAimRadius_, 2, 1);
    layout->addWidget(lblAimRadiusValue_, 2, 2);

    layout->addWidget(new QLabel(tr("Crosshair color") + ":", group), 3, 0);
    layout->addWidget(btnAimColor_, 3, 1);
    layout->addWidget(colorPreview_, 3, 2);

    group->setLayout(layout);

    return group;
}

void SettingsWidget::applyStyles()
{
    setStyleSheet("QLabel { background-color: #FCFCFC; }");
}

void SettingsWidget::connectSignals()
{
    connect(btnOpen_, &QPushButton::clicked, this, &SettingsWidget::onOpenCameraClicked);
    connect(btnOpenAll_, &QPushButton::clicked, this, &SettingsWidget::onOpenAllCamerasClicked);
    connect(chkAutoOpen_, &QCheckBox::toggled, this, &SettingsWidget::onAutoOpenCamerasChanged);
    connect(comboDeviceType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsWidget::onGaugeTypeChanged);
    connect(comboUnit_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsWidget::onPressureUnitChanged);
    connect(comboAccuracy_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsWidget::onPrecisionClassChanged);
    connect(comboPrinter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsWidget::onPrinterChanged);
    connect(comboDisplacement_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsWidget::onDialLayoutChanged);
    connect(btnConnectCom_, &QPushButton::clicked, this, &SettingsWidget::onConnectComPortClicked);
    connect(chkDrawCrosshair_, &QCheckBox::toggled, this, &SettingsWidget::onDrawCrosshairChanged);

    // новые сигналы
    connect(sliderAimRadius_, &QSlider::valueChanged, this, &SettingsWidget::onAimRadiusChanged);
    connect(btnAimColor_, &QPushButton::clicked, this, &SettingsWidget::onAimColorPickRequested);

    connect(ServiceLocator::instance().cameraProcessor(), &CameraProcessor::cameraStrChanged,
            this, [&](const QString &newCameraStr) {
                editCameras_->setText(newCameraStr);
            });
}

void SettingsWidget::onOpenCameraClicked() {
    const QString cameraString = editCameras_->text();
    ServiceLocator::instance().cameraProcessor()->setCameraString(cameraString);
}

void SettingsWidget::onOpenAllCamerasClicked() {
    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    const int cameraLimit = cameraProcessor->availableCameraCount();
    std::vector<qint32> indices;
    for (int i = 1; i <= cameraLimit; ++i) indices.push_back(i);
    cameraProcessor->setCameraIndices(indices);
}

void SettingsWidget::onAutoOpenCamerasChanged(bool checked) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_SYS_AUTOOPEN, checked);
}

void SettingsWidget::onGaugeTypeChanged(int index) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_GAUGE_MODEL, index);
}

void SettingsWidget::onPressureUnitChanged(int index) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRESSURE_UNIT, index);
    ServiceLocator::instance().partyManager()->setCurrentPressureUnit(index);
}

void SettingsWidget::onPrecisionClassChanged(int index) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRECISION_CLASS, index);
    ServiceLocator::instance().partyManager()->setCurrentPrecision(index);
}

void SettingsWidget::onPrinterChanged(int index) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRINTER, index);
    ServiceLocator::instance().partyManager()->setCurrentPrinter(index);
}

void SettingsWidget::onDialLayoutChanged(int index) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_DIAL_LAYOUT, index);
    ServiceLocator::instance().partyManager()->setCurrentDisplacement(index);
}

void SettingsWidget::onConnectComPortClicked() {
    QString err;
    const QString comPort = editComPort_->text();
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_PRESSURE_SENSOR_COM, comPort);
    bool isOpen = ServiceLocator::instance().pressureSensor()->openCOM(comPort, err);
    if (!isOpen) {
        QMessageBox::warning(
            this,
            tr("Failed to connect to pressure sensor"),
            (tr("Could not open COM port for the pressure sensor.") + "\n%1").arg(err)
        );
    }
}

void SettingsWidget::onDrawCrosshairChanged(bool checked) {
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_AIM_VISIBLE, checked);
    ServiceLocator::instance().cameraProcessor()->setAimEnabled(checked);
}

// ------------------------------
// НОВОЕ: изменение радиуса
// ------------------------------
void SettingsWidget::onAimRadiusChanged(int value)
{
    lblAimRadiusValue_->setText(QString::number(value));
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_AIM_RADIUS, value);
    ServiceLocator::instance().cameraProcessor()->setAimRadius(value);
}

// ------------------------------
// НОВОЕ: Color Picker
// ------------------------------
void SettingsWidget::onAimColorPickRequested()
{
    QColor newColor = QColorDialog::getColor(currentAimColor_, this, tr("Select crosshair color"));

    if (!newColor.isValid())
        return;

    currentAimColor_ = newColor;

    colorPreview_->setStyleSheet(QString("background-color: %1").arg(newColor.name()));

    ServiceLocator::instance().configManager()->setValue(CFG_KEY_AIM_COLOR, newColor.name());

    ServiceLocator::instance().cameraProcessor()->setAimColor(newColor);
}

void SettingsWidget::installEventFilters() {
    comboDeviceType_->installEventFilter(this);
    comboUnit_->installEventFilter(this);
    comboAccuracy_->installEventFilter(this);
    comboPrinter_->installEventFilter(this);
    comboDisplacement_->installEventFilter(this);
}

bool SettingsWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        if (qobject_cast<QComboBox*>(obj)) {
            event->ignore();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
