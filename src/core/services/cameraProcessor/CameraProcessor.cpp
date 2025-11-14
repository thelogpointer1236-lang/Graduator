#include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"
#include "dshow/FrameGrabberCB.h"
#include "CameraProcessor.h"
#include "defines.h"
#include "core/services/ServiceLocator.h"
CameraProcessor::CameraProcessor(QObject *parent) : QObject(parent) {
    videoProcessors.reserve(8);
}
CameraProcessor::~CameraProcessor() = default;
void CameraProcessor::setCameraString(const QString &cameraStr) {
    const int cameraLimit = availableCameraCount();
    auto cleanAndUnique = [cameraLimit](const QString &input) -> QString {
        QString digits;
        for (QChar c: input) {
            if (c.isDigit() && c != '0' && c.digitValue() <= cameraLimit && !digits.contains(c))
                digits.append(c);
        }
        return digits;
    };
    const QString camClean = cleanAndUnique(cameraStr);
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_CAMERA_STR, camClean);
    emit cameraStrChanged(camClean);
}
void CameraProcessor::setCameraIndices(std::vector<qint32> indices) {
    QString cameraStr;
    const int cameraLimit = availableCameraCount();
    std::sort(indices.begin(), indices.end());
    for (qint32 idx: indices) {
        if (idx > 0 && idx <= cameraLimit) {
            cameraStr.append(QString::number(idx));
        }
    }
    setCameraString(cameraStr);
}
int CameraProcessor::availableCameraCount() {
    return VideoCaptureProcessor::getCameraCount();
}
void CameraProcessor::enableDrawingCrosshair(bool enabled) {
    FrameGrabberCB::s_isDrawAim = enabled;
}
QString CameraProcessor::cameraStr() {
    return ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_CURRENT_CAMERA_STR, "");
}
QString CameraProcessor::sysCameraStr() {
    return ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_SYS_CAMERA_STR, "");
}
namespace {
    std::vector<qint32> extractDigits(const QString &input) {
        std::vector<qint32> result;
        result.reserve(input.size());
        for (QChar c: input) {
            if (c.isDigit()) {
                result.push_back(static_cast<quint32>(c.digitValue()));
            }
        }
        return result;
    }
}
std::vector<qint32> CameraProcessor::cameraIndices() {
    return extractDigits(cameraStr());
}
std::vector<qint32> CameraProcessor::sysCameraIndices() {
    return extractDigits(sysCameraStr());
}
void CameraProcessor::startAll() {
    FrameGrabberCB::s_isPolling = true;
}
void CameraProcessor::stopAll() {
    FrameGrabberCB::s_isPolling = false;
}