#include "CameraGridWidget.h"

#include "core/services/ServiceLocator.h"
#include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"

#include <QResizeEvent>
#include <QSizePolicy>
#include <QDebug>

namespace {
constexpr int CameraColumns = 2;
constexpr int CameraRows = 4;
constexpr int MinCameraWidth = 80;
constexpr int MinCameraHeight = 60;
constexpr double CameraAspectRatio = 4.0 / 3.0;
}

CameraGridWidget::CameraGridWidget(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
    createCameraLabels();
    setupConnections();
}

void CameraGridWidget::setupLayout()
{
    gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(gridLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CameraGridWidget::setupConnections()
{
    connect(ServiceLocator::instance().cameraProcessor(), &CameraProcessor::cameraStrChanged,
            this, &CameraGridWidget::onCameraStrChanged);
}

void CameraGridWidget::createCameraLabels()
{
    int index = 0;
    for (int column = 0; column < CameraColumns; ++column) {
        for (int row = 0; row < CameraRows; ++row) {
            auto *label = createCameraLabel(index);
            gridLayout->addWidget(label, row, column);
            cameraLabels.push_back(label);
            ++index;
        }
    }
}

AspectRatioLabel *CameraGridWidget::createCameraLabel(int index)
{
    auto *label = new AspectRatioLabel((tr("Camera") + " %1").arg(index + 1), this);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(MinCameraWidth, MinCameraHeight);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setAspectRatio(CameraAspectRatio);
    return label;
}

void CameraGridWidget::clearCameraLabels()
{
    for (auto *label : cameraLabels) {
        gridLayout->removeWidget(label);
        delete label;
    }
    cameraLabels.clear();
}

void CameraGridWidget::refresh()
{
    clearCameraLabels();
    createCameraLabels();
    updateLayout();
}

void CameraGridWidget::updateLayout()
{
    if (layout()) {
        layout()->invalidate();
        layout()->activate();
    }
    adjustSize();
    update();
}

void CameraGridWidget::onCameraStrChanged(const QString &newCameraStr)
{
    Q_UNUSED(newCameraStr);
    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    if (!cameraProcessor) return;

    const auto sysIndices = cameraProcessor->sysCameraIndices();
    const auto userIndices = cameraProcessor->cameraIndices();
    const int camCount = userIndices.size();

    refresh();
    rebuildVideoProcessors(camCount, userIndices, sysIndices);
}

void CameraGridWidget::rebuildVideoProcessors(int camCount,
                                              const std::vector<int> &userIndices,
                                              const std::vector<int> &sysIndices)
{
    auto *cameraProcessor = ServiceLocator::instance().cameraProcessor();
    if (!cameraProcessor) return;
    cameraProcessor->closeCameras();

    if (cameraLabels.empty() || sysIndices.empty()) return;

    for (int i = 0; i < camCount; ++i) {
        const int userIdx = userIndices[i];
        if (userIdx < 1 || userIdx > static_cast<int>(cameraLabels.size()) ||
            userIdx > static_cast<int>(sysIndices.size())) {
            continue;
        }
        auto *label = cameraLabels[userIdx - 1];
        if (!label) continue;
        const HWND hwnd = reinterpret_cast<HWND>(label->winId());
        const int sysIdx = sysIndices[userIdx - 1] - 1;
        Camera& cam = cameraProcessor->openCamera(hwnd, sysIdx);
        if (QString err; !cam.isOk(err)) {
            // TODO: handle error
        }
        cam.resizeVideoWindow(getCameraWindowSize());
    }
    cameraProcessor->emitCamerasChanged();
}

void CameraGridWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    for (Camera& cam : ServiceLocator::instance().cameraProcessor()->cameras())
        cam.resizeVideoWindow(getCameraWindowSize());
}

QSize CameraGridWidget::getCameraWindowSize() const
{
    return {this->size().width() / CameraColumns, this->size().height() / CameraRows};
}
