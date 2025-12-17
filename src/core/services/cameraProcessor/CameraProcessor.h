#ifndef GRADUATOR_CAMERAPROCESSOR_H
#define GRADUATOR_CAMERAPROCESSOR_H
#include "Camera.h"
#include <QObject>
#include <QString>
#include <vector>
#include <map>
#include <set>

class CameraProcessor final : public QObject {
    Q_OBJECT
public:
    explicit CameraProcessor(QObject *parent = nullptr);
    ~CameraProcessor() override;
    void setCameraString(const QString &cameraStr);
    void setCameraIndices(std::vector<qint32> indices);
    Camera& openCamera(void *hwnd, int cameraIndex);
    std::vector<Camera>& cameras();
    void closeCameras();

    static void setCapRate(int rate);
    static void restoreDefaultCapRate();

    qreal lastAngleForCamera(qint32 cameraIdx) const;

    static void setAimEnabled(bool enabled);
    static void setCapturingRate(int rate);
    static void setAimColor(const QColor &color);
    static void setAimRadius(int radius);

    static int availableCameraCount();
    static QString cameraStr();
    static QString sysCameraStr();
    static std::vector<qint32> cameraIndices();
    static std::vector<qint32> sysCameraIndices();

    void emitCamerasChanged();

private slots:
    void onAngleMeasured(qint32 idx, qreal time, qreal angle);

signals:
    void angleMeasured(qint32 idx, qreal time, qreal angle);
    void cameraStrChanged(const QString &newCameraStr);
    void camerasChanged();

private:

    std::vector<Camera> m_cameras;
    std::map<qint32, qreal> m_lastAngles;
    std::set<qint32> m_angleOvershootAcknowledged;
};

#endif //GRADUATOR_CAMERAPROCESSOR_H
