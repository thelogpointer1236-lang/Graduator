#ifndef GRADUATOR_CAMERAPROCESSOR_H
#define GRADUATOR_CAMERAPROCESSOR_H
#include "anglemeter/AnglemeterProcessor.h"
#include <QObject>
#include <QString>
#include <vector>

class VideoCaptureProcessor;
class CameraProcessor final : public QObject {
    Q_OBJECT
public:
    explicit CameraProcessor(int anglemeterThreadsCount, int imgWidth, int imgHeight, QObject *parent = nullptr);
    ~CameraProcessor() override;
    void setCameraString(const QString &cameraStr);
    void setCameraIndices(std::vector<qint32> indices);
    VideoCaptureProcessor* createVideoProcessor(QObject *parent = nullptr);
    const std::vector<VideoCaptureProcessor*>& videoProcessors();
    void clearVideoProcessors();
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

private slots:
    void enqueueImage(qint32 cameraIdx, qreal time, quint8* imgData);
    void onAngleMeasured(qint32 idx, qreal time, qreal angle);

signals:
    void angleMeasured(qint32 idx, qreal time, qreal angle);
    void cameraStrChanged(const QString &newCameraStr);

private:
    std::vector<VideoCaptureProcessor*> m_videoProcessors;
    std::vector<AnglemeterProcessor*> m_anglemeterProcessors;
    std::vector<QThread*> m_anglemeterThreads;
    std::map<qint32, qreal> m_lastAngles;
};

#endif //GRADUATOR_CAMERAPROCESSOR_H
