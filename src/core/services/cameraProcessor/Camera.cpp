#include "Camera.h"
#include "dshow/VideoCaptureProcessor.h"

Camera::Camera() {

    video_ = std::make_shared<VideoCaptureProcessor>();
    settings_.setVideoCapProcessor(video_.get());

}

Camera::Camera(const Camera & other) {
    video_ = other.video_;
    settings_ = other.settings_;
    err_ = other.err_;
}

Camera::~Camera() { }

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

VideoCaptureProcessor * Camera::video() const { return video_.get(); }
