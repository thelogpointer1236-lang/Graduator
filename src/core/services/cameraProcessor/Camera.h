#ifndef GRADUATOR_CAMERA_H
#define GRADUATOR_CAMERA_H
#include <QSize>
#include <QString>
#include "CameraSettings.h"

class VideoCaptureProcessor;
class Camera {
public:
    explicit Camera();
    ~Camera();

    bool isOk(QString& err) const;
    void init(void *hwnd, int cameraIndex);
    void resizeVideoWindow(const QSize &size);
    CameraSettings* settings();
    VideoCaptureProcessor *video() { return video_; }

private:
    VideoCaptureProcessor *video_;
    CameraSettings settings_;
    QString err_;
};

#endif //GRADUATOR_CAMERA_H