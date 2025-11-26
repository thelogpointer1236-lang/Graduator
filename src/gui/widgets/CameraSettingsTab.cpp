#include "CameraSettingsTab.h"

#include "core/services/ServiceLocator.h"
#include "core/services/cameraProcessor/CameraProcessor.h"
#include "core/services/cameraProcessor/Camera.h"
#include "core/services/cameraProcessor/CameraSettings.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

namespace {
constexpr int DefaultMinValue = 0;
constexpr int DefaultMaxValue = 100;
}

CameraSettingsTab::CameraSettingsTab(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    if (auto *cameraProcessor = ServiceLocator::instance().cameraProcessor()) {
        connect(cameraProcessor, &CameraProcessor::camerasChanged,
                this, &CameraSettingsTab::rebuildCameraSettings);
        rebuildCameraSettings();
    }
}

void CameraSettingsTab::setupUi()
{
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    auto *scrollWidget = new QWidget(scrollArea);
    camerasLayout_ = new QVBoxLayout(scrollWidget);
    camerasLayout_->setContentsMargins(0, 0, 0, 0);
    camerasLayout_->setSpacing(10);
    scrollWidget->setLayout(camerasLayout_);

    scrollArea->setWidget(scrollWidget);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);
}

void CameraSettingsTab::clearCameraGroups()
{
    while (auto *item = camerasLayout_->takeAt(0)) {
        if (auto *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void CameraSettingsTab::rebuildCameraSettings()
{
    clearCameraGroups();

    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    if (!cameraProcessor) {
        return;
    }

    auto &cameras = cameraProcessor->cameras();
    if (cameras.empty()) {
        camerasLayout_->addWidget(new QLabel(tr("No cameras available")));
        camerasLayout_->addStretch();
        return;
    }

    for (int i = 0; i < static_cast<int>(cameras.size()); ++i) {
        auto *cameraGroup = createCameraGroup(i);
        if (cameraGroup) {
            camerasLayout_->addWidget(cameraGroup);
        }
    }

    camerasLayout_->addStretch();
}

QGroupBox *CameraSettingsTab::createCameraGroup(int cameraIndex)
{
    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    if (!cameraProcessor) return nullptr;

    auto &cameras = cameraProcessor->cameras();
    if (cameraIndex < 0 || cameraIndex >= static_cast<int>(cameras.size())) return nullptr;

    Camera &camera = cameras[static_cast<size_t>(cameraIndex)];
    auto *settings = camera.settings();
    if (!settings) return nullptr;

    QVector<QString> keys;
    settings->getAvailableKeys(keys);

    auto *group = new QGroupBox(tr("Camera %1").arg(cameraIndex + 1));
    auto *layout = new QVBoxLayout(group);

    for (const auto &key : keys) {
        long min = DefaultMinValue, max = DefaultMaxValue;
        bool rangeOk = false;
        settings->getKeyRange(key, min, max, &rangeOk);

        bool valueOk = false;
        const long current = settings->getValue(key, &valueOk);

        const long startValue = valueOk ? current : min;
        layout->addWidget(createSettingRow(cameraIndex, key,
                                           rangeOk ? min : DefaultMinValue,
                                           rangeOk ? max : DefaultMaxValue,
                                           startValue));
    }

    return group;
}

QWidget *CameraSettingsTab::createSettingRow(int cameraIndex, const QString &key, long min, long max, long current)
{
    auto *rowWidget = new QWidget;
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    auto *label = new QLabel(key + ":", rowWidget);
    auto *valueLabel = new QLabel(QString::number(current), rowWidget);

    auto *slider = new QSlider(Qt::Horizontal, rowWidget);
    slider->setRange(static_cast<int>(min), static_cast<int>(max));
    slider->setValue(static_cast<int>(current));

    connect(slider, &QSlider::valueChanged, this, [this, cameraIndex, key, valueLabel](int value) {
        valueLabel->setText(QString::number(value));
        applySetting(cameraIndex, key, value);
    });

    rowLayout->addWidget(label, 1);
    rowLayout->addWidget(slider, 3);
    rowLayout->addWidget(valueLabel);

    return rowWidget;
}

void CameraSettingsTab::applySetting(int cameraIndex, const QString &key, int value)
{
    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    if (!cameraProcessor) return;

    auto &cameras = cameraProcessor->cameras();
    if (cameraIndex < 0 || cameraIndex >= static_cast<int>(cameras.size())) return;

    auto *settings = cameras[static_cast<size_t>(cameraIndex)].settings();
    if (!settings) return;

    settings->setValue(key, static_cast<long>(value));
}

