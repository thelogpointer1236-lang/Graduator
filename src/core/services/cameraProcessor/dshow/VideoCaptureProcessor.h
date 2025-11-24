#ifndef VIDEOCAPTUREPROCESSOR_H
#define VIDEOCAPTUREPROCESSOR_H
#include <QObject>
#include <QString>
#include <QPair>
class FrameGrabberCB;
class VideoCaptureProcessor final : public QObject {
    Q_OBJECT
public:
    explicit VideoCaptureProcessor(QObject *parent = nullptr);
    ~VideoCaptureProcessor() override;
    bool init(void *hwnd, int cameraIndex);
    static int getCameraCount();
    void resizeVideoWindow(const QSize &size);
    FrameGrabberCB *frameGrabberCB() const;

signals:
    void imageCaptured(qint32 cameraIdx, qreal time, quint8* imgData);

private:
    void checkHR(long hr, const QString &errorMessage);
    void cleanup();
    // COM interfaces
    class IGraphBuilder *m_pGraph;
    class ICaptureGraphBuilder2 *m_pCapture;
    class IBaseFilter *m_pSrcFilter;
    class IBaseFilter *m_pSampleGrabberFilter;
    class ISampleGrabber *m_pSampleGrabber;
    class IBaseFilter *m_pVideoRenderer;
    class IMediaControl *m_pMediaControl;
    class IVideoWindow *m_pVideoWindow;
    class IEnumMoniker *m_pEnum;
    class IMoniker *m_pMoniker;
    class ICreateDevEnum *m_pDevEnum;
    // Window handle
    void *m_hwnd;
    // Camera index
    int m_cameraIndex;
    // COM initialization flag
    bool m_comInitialized;
    FrameGrabberCB *m_grabberCB;
};
#endif //VIDEOCAPTUREPROCESSOR_H