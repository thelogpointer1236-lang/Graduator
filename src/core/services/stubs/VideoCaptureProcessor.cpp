#ifdef USE_STUB_IMPLEMENTATIONS

#include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"

#include <QByteArray>
#include <QDateTime>
#include <QPointer>
#include <QSharedPointer>
#include <QSize>
#include <QTimer>
#include <QImage>
#include <QDebug>

VideoCaptureProcessor::VideoCaptureProcessor(QObject *parent)
    : QObject(parent)
      , m_pGraph(nullptr)
      , m_pCapture(nullptr)
      , m_pSrcFilter(nullptr)
      , m_pSampleGrabberFilter(nullptr)
      , m_pSampleGrabber(nullptr)
      , m_pVideoRenderer(nullptr)
      , m_pMediaControl(nullptr)
      , m_pVideoWindow(nullptr)
      , m_pEnum(nullptr)
      , m_pMoniker(nullptr)
      , m_pDevEnum(nullptr)
      , m_pCameraControl(nullptr)
      , m_pVideoProcAmp(nullptr)
      , m_hwnd(nullptr)
      , m_cameraIndex(0)
      , m_comInitialized(false)
      , m_grabberCB(nullptr) {
}

VideoCaptureProcessor::~VideoCaptureProcessor() = default;

void VideoCaptureProcessor::init(void *hwnd, int cameraIndex)
{
    m_hwnd = hwnd;
    m_cameraIndex = cameraIndex;
    m_comInitialized = true;

    // Загружаем stub-картинку из ресурсов
    QImage img(":/assets/assets/stub_arrow.png");
    if (img.isNull()) {
        qWarning() << "Stub-image not found!";
        return;
    }

    // Приводим к формату RGB888 (3 байта на пиксель)
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);

    // Копируем в QByteArray
    auto buffer = QSharedPointer<QByteArray>::create(
        reinterpret_cast<const char*>(rgb.constBits()),
        rgb.width() * rgb.height() * 3
    );

    QPointer<VideoCaptureProcessor> self(this);

    auto timer = new QTimer(this);
    timer->setInterval(250);

    connect(timer, &QTimer::timeout, this, [self, buffer]() {
        if (!self)
            return;

        const qreal timestampSec =
            QDateTime::currentMSecsSinceEpoch() / 1000.0;

        emit self->imageCaptured(
            self->m_cameraIndex,
            timestampSec,
            reinterpret_cast<const quint8*>(buffer->constData())
        );
    });

    timer->start();
}


int VideoCaptureProcessor::getCameraCount() {
    return 1;
}

void VideoCaptureProcessor::resizeVideoWindow(const QSize &size) {
    Q_UNUSED(size);
}

FrameGrabberCB *VideoCaptureProcessor::frameGrabberCB() const {
    return nullptr;
}

void VideoCaptureProcessor::debugCameraControls() {
}

void VideoCaptureProcessor::debugVideoProcAmp() {
}

void VideoCaptureProcessor::debugAllCameraProps() {
}

bool VideoCaptureProcessor::getVideoProcAmpCurrent(long prop, long &current, long &flags) {
    Q_UNUSED(prop);
    current = 0;
    flags = 0;
    return true;
}

bool VideoCaptureProcessor::setVideoProcAmpProperty(long prop, long value) {
    Q_UNUSED(prop);
    Q_UNUSED(value);
    return true;
}

bool VideoCaptureProcessor::getVideoProcAmpRange(long prop, long &min, long &max) {
    Q_UNUSED(prop);
    min = 0;
    max = 100;
    return true;
}

#endif
