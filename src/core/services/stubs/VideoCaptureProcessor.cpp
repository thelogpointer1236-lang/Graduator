// #ifdef USE_STUB_IMPLEMENTATIONS
//
// #include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"
// #include <QTimer>
// #include <QElapsedTimer>
// #include <QDebug>
//
// namespace {
//
//     constexpr qreal ANGLE_MIN = 40.0;     // нижний предел
//     constexpr qreal ANGLE_MAX = 320.0;    // верхний предел
//
//     constexpr int PERIOD_MS   = 50 * 1000;     // 50 секунд вверх или вниз
//     constexpr int TICK_MS     = 100;           // таймер
//
//     QElapsedTimer g_timer;
//     int g_stubCameraCounter = 0;
//
// }
//
// VideoCaptureProcessor::VideoCaptureProcessor(QObject *parent)
//     : QObject(parent)
//     , m_pGraph(nullptr)
//     , m_pCapture(nullptr)
//     , m_pSrcFilter(nullptr)
//     , m_pSampleGrabberFilter(nullptr)
//     , m_pSampleGrabber(nullptr)
//     , m_pVideoRenderer(nullptr)
//     , m_pMediaControl(nullptr)
//     , m_pVideoWindow(nullptr)
//     , m_pEnum(nullptr)
//     , m_pMoniker(nullptr)
//     , m_pDevEnum(nullptr)
//     , m_hwnd(nullptr)
//     , m_cameraIndex(++g_stubCameraCounter)
//     , m_comInitialized(false)
//     , m_grabberCB(nullptr)
// {
//     g_timer.start();   // запускаем измерение настоящего времени
//
//     auto timer = new QTimer(this);
//     timer->setInterval(TICK_MS);
//
//     connect(timer, &QTimer::timeout, this, [this]() {
//
//         const qint64 elapsedMs = g_timer.elapsed();   // строгое реальное время
//
//         constexpr int upMs   = PERIOD_MS;   // 50 сек вверх
//         constexpr int downMs = PERIOD_MS;   // 50 сек вниз
//         constexpr int cycle  = upMs + downMs; // полный цикл 100 сек
//
//         const qint64 t = elapsedMs % cycle;
//
//         qreal angle = 0.0;
//
//         if (t < upMs) {
//             // Рост MIN → MAX
//             const qreal k = qreal(t) / qreal(upMs);
//             angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * k;
//         } else {
//             // Спад MAX → MIN
//             const qint64 t2 = t - upMs;
//             const qreal k = qreal(t2) / qreal(downMs);
//             angle = ANGLE_MAX - (ANGLE_MAX - ANGLE_MIN) * k;
//         }
//
//         // небольшой сдвиг между камерами (можно убрать)
//         const qreal shift = m_cameraIndex * 2.0;
//         const qreal finalAngle = angle + shift;
//
//         emit angleReady(m_cameraIndex,
//                         elapsedMs / 1000.0,   // время в секундах
//                         finalAngle);
//     });
//
//     timer->start();
// }
//
// VideoCaptureProcessor::~VideoCaptureProcessor() = default;
//
// bool VideoCaptureProcessor::init(void *hwnd, int cameraIndex)
// {
//     m_hwnd = hwnd;
//     m_cameraIndex = cameraIndex;
//     return true;
// }
//
// int VideoCaptureProcessor::getCameraCount()
// {
//     return 8;
// }
//
// void VideoCaptureProcessor::resizeVideoWindow(const QSize &)
// {
// }
//
// FrameGrabberCB* VideoCaptureProcessor::frameGrabberCB() const
// {
//     return nullptr;
// }
//
// void VideoCaptureProcessor::checkHR(long, const QString &)
// {
// }
//
// void VideoCaptureProcessor::cleanup()
// {
// }
//
// #endif
