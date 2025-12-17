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
    void init(void *hwnd, int cameraIndex);
    static int getCameraCount();
    void resizeVideoWindow(const QSize &size);
    FrameGrabberCB *frameGrabberCB() const;

    void debugCameraControls();
    void debugVideoProcAmp();
    void debugAllCameraProps();

    bool getVideoProcAmpCurrent(long prop, long& current, long& flags);
    bool setVideoProcAmpProperty(long prop, long value);
    bool getVideoProcAmpRange(long prop, long& min, long& max);

signals:
    void angleMeasured(qint32 cameraIdx, qreal time, qreal angle);

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
    class IAMCameraControl *m_pCameraControl;
    class IAMVideoProcAmp *m_pVideoProcAmp;
    // Window handle
    void *m_hwnd;
    // Camera index
    int m_cameraIndex;
    // COM initialization flag
    bool m_comInitialized;
    FrameGrabberCB *m_grabberCB;
};
#endif //VIDEOCAPTUREPROCESSOR_H
