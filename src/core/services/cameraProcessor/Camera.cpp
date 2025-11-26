#include "Camera.h"
#include "dshow/VideoCaptureProcessor.h"

Camera::Camera()
    : video_(new VideoCaptureProcessor())
    , settings_(video_){
}

Camera::~Camera() {
    delete video_;
}

bool Camera::isOk(QString& err) const {
    err = err_;
    return err_.isEmpty();
}

void Camera::init(void *hwnd, int cameraIndex) {
    try {
        video_->init(hwnd, cameraIndex);
    }
    catch (const QString& e) {
        err_ = e;
        return;
    }
    err_ = QString();
}

void Camera::resizeVideoWindow(const QSize &size) {
    video_->resizeVideoWindow(size);
}

CameraSettings * Camera::settings() {
    if (isOk(err_)) return &settings_;
    return nullptr;
}
