#include "VideoCaptureProcessor.h"
#include "FrameGrabberCB.h"
#include "core/services/ServiceLocator.h"
#include <QtDebug>
#include <QSize>
VideoCaptureProcessor::VideoCaptureProcessor(QObject *parent)
    : QObject(parent)
      , m_hwnd(nullptr)
      , m_cameraIndex(0)
      , m_comInitialized(false)
      , m_grabberCB(nullptr)
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
      , m_pDevEnum(nullptr) {
}
VideoCaptureProcessor::~VideoCaptureProcessor() {
    cleanup();
}
FrameGrabberCB *VideoCaptureProcessor::frameGrabberCB() const {
    return m_grabberCB;
}
void VideoCaptureProcessor::checkHR(const HRESULT hr, const QString &errorMessage) {
    if (FAILED(hr) || hr == S_FALSE) {
        ServiceLocator::instance().logger()->error(QString("VideoCaptureProcessor error: %1. HRESULT = 0x%2")
            .arg(errorMessage)
            .arg(static_cast<qulonglong>(hr), 8, 16, QChar('0')));
    }
}
int VideoCaptureProcessor::getCameraCount() {
    HRESULT hr = S_OK;
    ComPtr<IEnumMoniker> pEnum;
    ComPtr<ICreateDevEnum> pDevEnum;
    int count = 0;
    // Initialize COM
    const bool comInit = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY));
    if (!comInit) {
        return 0;
    }
    // Create device enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
    if (FAILED(hr)) {
        CoUninitialize();
        return 0;
    }
    // Get enumerator for video capture devices
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, pEnum.GetAddressOf(), 0);
    if (hr == S_FALSE || pEnum == nullptr) {
        CoUninitialize();
        return 0;
    } else if (FAILED(hr)) {
        CoUninitialize();
        return 0;
    }
    // Count devices
    ULONG cFetched = 0;
    ComPtr<IMoniker> pMoniker;
    while (pEnum->Next(1, pMoniker.GetAddressOf(), &cFetched) == S_OK && cFetched > 0) {
        count++;
        pMoniker.Reset();
    }
    CoUninitialize();
    return count;
}
void VideoCaptureProcessor::resizeVideoWindow(const QSize &size) {
    if (!m_pVideoWindow) {
        // ServiceLocator::instance().logger()->error(L"VideoCaptureProcessor error: Интерфейс видеоокна не инициализирован.");
        return;
    }
    // Call COM methods via a helper function
    m_pVideoWindow->SetWindowPosition(0, 0, size.width(), size.height());
}
bool VideoCaptureProcessor::init(void *hwnd, int cameraIndex) {
    // Присваиваем переданные параметры членам класса
    m_hwnd = hwnd;
    m_cameraIndex = cameraIndex;
    HRESULT hr = S_OK;
    // 1. Инициализация COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(hr)) {
        checkHR(hr, "Failed to CoInitializeEx");
        return false;
    }
    m_comInitialized = true;
    // 2. Создание Filter Graph Manager и Capture Graph Builder
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pGraph));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to create Filter Graph Manager");
        cleanup();
        return false;
    }
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pCapture));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to create Capture Graph Builder");
        cleanup();
        return false;
    }
    hr = m_pCapture->SetFiltergraph(m_pGraph);
    if (FAILED(hr)) {
        checkHR(hr, "Failed to set filter graph");
        cleanup();
        return false;
    }
    // 3. Перечисление и выбор устройства видеозахвата
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDevEnum));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to create System Device Enumerator");
        cleanup();
        return false;
    }
    hr = m_pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &m_pEnum, 0);
    if (FAILED(hr) || hr == S_FALSE) {
        checkHR(hr, "Failed to create video input device enumerator or no devices found");
        cleanup();
        return false;
    }
    // Поиск устройства по индексу
    bool deviceFound = false;
    int currentIndex = 0;
    ULONG cFetched = 0;
    while (m_pEnum->Next(1, &m_pMoniker, &cFetched) == S_OK && cFetched > 0) {
        if (currentIndex == m_cameraIndex) {
            deviceFound = true;
            break;
        }
        currentIndex++;
        m_pMoniker->Release(); // Освобождаем моникер, если он нам не нужен
        m_pMoniker = nullptr;
    }
    if (!deviceFound) {
        checkHR(E_FAIL, QString("Camera with index %1 not found.").arg(m_cameraIndex));
        cleanup();
        return false;
    }
    // 4. Создание фильтра-источника для выбранного устройства
    ComPtr<IPropertyBag> pPropBag;
    hr = m_pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to bind moniker to storage");
        // cleanup();
        return false;
    }
    hr = m_pMoniker->BindToObject(NULL, NULL, IID_PPV_ARGS(&m_pSrcFilter));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to bind to video capture device");
        // cleanup();
        return false;
    }
    hr = m_pGraph->AddFilter(m_pSrcFilter, L"Video Capture Source");
    if (FAILED(hr)) {
        checkHR(hr, "Failed to add source filter to graph");
        // cleanup();
        return false;
    }
    // 5. Создание и настройка фильтра Sample Grabber
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pSampleGrabberFilter));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to create SampleGrabber filter");
        // cleanup();
        return false;
    }
    hr = m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Sample Grabber");
    if (FAILED(hr)) {
        checkHR(hr, "Failed to add SampleGrabber to graph");
        // cleanup();
        return false;
    }
    hr = m_pSampleGrabberFilter->QueryInterface(IID_PPV_ARGS(&m_pSampleGrabber));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to get ISampleGrabber interface");
        // cleanup();
        return false;
    }
    // Настройка типа медиа для Sample Grabber (например, 24-битный RGB)
    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(VIDEOINFOHEADER);
    mt.pbFormat = (BYTE *) CoTaskMemAlloc(mt.cbFormat);
    hr = m_pSampleGrabber->SetMediaType(&mt);
    if (FAILED(hr)) {
        checkHR(hr, "Failed to set media type on SampleGrabber");
        cleanup();
        return false;
    }
    // Создание и установка callback
    m_grabberCB = new FrameGrabberCB(m_cameraIndex);
    // Предполагая, что FrameGrabberCB - ваша реализация ISampleGrabberCB
    connect(m_grabberCB, &FrameGrabberCB::angleReady, this, &VideoCaptureProcessor::angleReady, Qt::QueuedConnection);
    // m_callback->AddRef(); // Ручное увеличение счетчика ссылок
    hr = m_pSampleGrabber->SetCallback(m_grabberCB, 1); // Метод 1 для BufferCB
    if (FAILED(hr)) {
        checkHR(hr, "Failed to set callback on SampleGrabber");
        // cleanup();
        return false;
    }
    // 6. Создание фильтра Video Renderer
    hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pVideoRenderer));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to create Video Renderer");
        // cleanup();
        return false;
    }
    hr = m_pGraph->AddFilter(m_pVideoRenderer, L"Video Renderer");
    if (FAILED(hr)) {
        checkHR(hr, "Failed to add Video Renderer to graph");
        // cleanup();
        return false;
    }
    // 7. Рендеринг потока, соединение фильтров
    hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pSrcFilter, m_pSampleGrabberFilter,
                                  m_pVideoRenderer);
    if (FAILED(hr)) {
        // Если категория захвата не удалась, пробуем категорию предварительного просмотра в качестве запасного варианта
        hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_pSrcFilter, m_pSampleGrabberFilter,
                                      m_pVideoRenderer);
        if (FAILED(hr)) {
            checkHR(hr, "Failed to render video stream for both capture and preview pins");
            // cleanup();
            return false;
        }
    }
    // 8. Получение интерфейсов управления
    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pMediaControl));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to get IMediaControl interface");
        // cleanup();
        return false;
    }
    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pVideoWindow));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to get IVideoWindow interface");
        // cleanup();
        return false;
    }
    // 9. Настройка видео окна
    hr = m_pVideoWindow->put_Owner(reinterpret_cast<OAHWND>(m_hwnd));
    if (FAILED(hr)) {
        checkHR(hr, "Failed to set video window owner");
        // cleanup();
        return false;
    }
    hr = m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    if (FAILED(hr)) {
        checkHR(hr, "Failed to set video window style");
        // cleanup();
        return false;
    }
    // Установка начального размера
    RECT rc;
    GetClientRect((HWND) m_hwnd, &rc);
    checkHR(
        m_pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom),
        "Failed to set video window position");
    // checkHR(
    //     m_pVideoWindow->put_Visible(OATRUE),
    //     "Failed to make video window visible");
    // 10. Запуск графа
    hr = m_pMediaControl->Run();
    if (FAILED(hr)) {
        checkHR(hr, "Failed to run the filter graph");
        // cleanup();
        return false;
    }
    return true; // Успех
}
template<class T>
inline void safeReleaseDirectShow(T **v) {
    if (*v) {
        (*v)->Release();
        *v = NULL;
    }
}
void VideoCaptureProcessor::cleanup() {
    // Остановка графа
    if (m_pMediaControl) {
        m_pMediaControl->Release();
    }
    // Очистка видео окна
    if (m_pVideoWindow) {
        m_pVideoWindow->Release();
        m_pVideoWindow = nullptr;
    }
    // Освобождение callback
    if (m_grabberCB) {
        m_grabberCB->Release();
        m_grabberCB = nullptr;
    }
    // Очистка COM объектов
    safeReleaseDirectShow(&m_pGraph);
    safeReleaseDirectShow(&m_pCapture);
    safeReleaseDirectShow(&m_pSrcFilter);
    safeReleaseDirectShow(&m_pSampleGrabberFilter);
    safeReleaseDirectShow(&m_pSampleGrabber);
    safeReleaseDirectShow(&m_pVideoRenderer);
    safeReleaseDirectShow(&m_pEnum);
    safeReleaseDirectShow(&m_pMoniker);
    safeReleaseDirectShow(&m_pDevEnum);
}