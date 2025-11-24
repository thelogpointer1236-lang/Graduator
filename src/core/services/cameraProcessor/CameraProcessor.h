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

    static int availableCameraCount();
    static void enableDrawingCrosshair(bool enabled);
    static QString cameraStr();
    static QString sysCameraStr();
    static std::vector<qint32> cameraIndices();
    static std::vector<qint32> sysCameraIndices();

private slots:
    void enqueueImage(qint32 cameraIdx, qreal time, quint8* imgData);

signals:
    void angleMeasured(qint32 idx, qreal time, qreal angle);
    void cameraStrChanged(const QString &newCameraStr);

private:
    std::vector<VideoCaptureProcessor*> m_videoProcessors;
    std::vector<AnglemeterProcessor*> m_anglemeterProcessors;
    std::vector<QThread*> m_anglemeterThreads;
};

#endif //GRADUATOR_CAMERAPROCESSOR_H
