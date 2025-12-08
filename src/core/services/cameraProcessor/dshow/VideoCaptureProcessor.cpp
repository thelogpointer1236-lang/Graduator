#include "VideoCaptureProcessor.h"
#include "FrameGrabberCB.h"
#include "core/services/ServiceLocator.h"
#include <QtDebug>
#include <QSize>

namespace {
    void FreeMediaType(AM_MEDIA_TYPE& mt)
    {
        if (mt.cbFormat != 0) {
            CoTaskMemFree((PVOID)mt.pbFormat);
            mt.cbFormat = 0;
            mt.pbFormat = NULL;
        }
        if (mt.pUnk != NULL) {
            // pUnk should be released last
            mt.pUnk->Release();
            mt.pUnk = NULL;
        }
    }

    void DeleteMediaType(AM_MEDIA_TYPE* pmt)
    {
        if (pmt == NULL) return;

        FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}

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
      , m_pDevEnum(nullptr)
      , m_pCameraControl(nullptr)
      , m_pVideoProcAmp(nullptr) {
}

VideoCaptureProcessor::~VideoCaptureProcessor() {
    cleanup();
}

FrameGrabberCB *VideoCaptureProcessor::frameGrabberCB() const {
    return m_grabberCB;
}


void VideoCaptureProcessor::checkHR(const HRESULT hr, const QString &errorMessage) {
    if (FAILED(hr) || hr == S_FALSE) {
        throw QString("VideoCaptureProcessor error: %1. HRESULT = 0x%2")
            .arg(errorMessage)
            .arg(static_cast<qulonglong>(hr), 8, 16, QChar('0'));
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
        return;
    }
    // Call COM methods via a helper function
    m_pVideoWindow->SetWindowPosition(0, 0, size.width(), size.height());
}

void VideoCaptureProcessor::init(void *hwnd, int cameraIndex) {
    m_hwnd = hwnd;
    m_cameraIndex = cameraIndex;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(hr)) {
        checkHR(hr, "CoInitializeEx failed");
    }
    m_comInitialized = true;

    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pGraph));
    if (FAILED(hr)) { checkHR(hr, "Failed to create FilterGraph"); cleanup();}

    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pCapture));
    if (FAILED(hr)) { checkHR(hr, "Failed to create CaptureGraphBuilder2"); cleanup();}

    hr = m_pCapture->SetFiltergraph(m_pGraph);
    if (FAILED(hr)) { checkHR(hr, "SetFiltergraph failed"); cleanup();}

    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDevEnum));
    if (FAILED(hr)) { checkHR(hr, "Failed to create SystemDeviceEnum"); cleanup();}

    hr = m_pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &m_pEnum, 0);
    if (FAILED(hr) || hr == S_FALSE) {
        checkHR(hr, "No video capture devices");
        cleanup();
    }

    ULONG fetched = 0;
    int idx = 0;
    while (m_pEnum->Next(1, &m_pMoniker, &fetched) == S_OK && fetched > 0) {
        if (idx == m_cameraIndex) break;
        idx++;
        m_pMoniker->Release();
        m_pMoniker = nullptr;
    }

    if (!m_pMoniker) {
        checkHR(E_FAIL, QString("Camera index %1 not found").arg(m_cameraIndex));
        cleanup();
    }

    hr = m_pMoniker->BindToObject(NULL, NULL, IID_PPV_ARGS(&m_pSrcFilter));
    if (FAILED(hr)) { checkHR(hr, "BindToObject failed"); cleanup();}

    hr = m_pGraph->AddFilter(m_pSrcFilter, L"Video Capture Source");
    if (FAILED(hr)) { checkHR(hr, "AddFilter Source failed"); cleanup();}


// ===================================================================

    // ---------- ВСТРОЕННЫЙ БЛОК: получение интерфейсов управления камерой ----------
    hr = m_pSrcFilter->QueryInterface(IID_PPV_ARGS(&m_pCameraControl));
    if (FAILED(hr)) m_pCameraControl = nullptr;

    hr = m_pSrcFilter->QueryInterface(IID_PPV_ARGS(&m_pVideoProcAmp));
    if (FAILED(hr)) m_pVideoProcAmp = nullptr;
    // ------------------------------------------------------------------------------

    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pSampleGrabberFilter));
    if (FAILED(hr)) { checkHR(hr, "Create SampleGrabber failed"); cleanup();}

    hr = m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Sample Grabber");
    if (FAILED(hr)) { checkHR(hr, "AddFilter SampleGrabber failed"); cleanup();}

    hr = m_pSampleGrabberFilter->QueryInterface(IID_PPV_ARGS(&m_pSampleGrabber));
    if (FAILED(hr)) { checkHR(hr, "QueryInterface SampleGrabber failed"); cleanup();}

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(VIDEOINFOHEADER);
    mt.pbFormat = (BYTE*)CoTaskMemAlloc(mt.cbFormat);

    hr = m_pSampleGrabber->SetMediaType(&mt);
    if (FAILED(hr)) { checkHR(hr, "SetMediaType failed"); cleanup();}

    m_grabberCB = new FrameGrabberCB(m_cameraIndex, 640, 480);
    connect(m_grabberCB, &FrameGrabberCB::imageCaptured, this,
            &VideoCaptureProcessor::imageCaptured, Qt::DirectConnection);

    hr = m_pSampleGrabber->SetCallback(m_grabberCB, 1);
    if (FAILED(hr)) { checkHR(hr, "SetCallback failed"); cleanup();}

    hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pVideoRenderer));
    if (FAILED(hr)) { checkHR(hr, "Create VideoRenderer failed"); cleanup();}

    hr = m_pGraph->AddFilter(m_pVideoRenderer, L"Video Renderer");
    if (FAILED(hr)) { checkHR(hr, "AddFilter VideoRenderer failed"); cleanup();}

    hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                  m_pSrcFilter, m_pSampleGrabberFilter, m_pVideoRenderer);
    if (FAILED(hr)) {
        hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                      m_pSrcFilter, m_pSampleGrabberFilter, m_pVideoRenderer);
        if (FAILED(hr)) {
            checkHR(hr, "RenderStream failed");
            cleanup();
        }
    }

    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pMediaControl));
    if (FAILED(hr)) { checkHR(hr, "QueryInterface MediaControl failed"); cleanup();}

    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pVideoWindow));
    if (FAILED(hr)) { checkHR(hr, "QueryInterface VideoWindow failed"); cleanup();}

    hr = m_pVideoWindow->put_Owner(reinterpret_cast<OAHWND>(m_hwnd));
    if (FAILED(hr)) { checkHR(hr, "put_Owner failed"); cleanup();}

    hr = m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    if (FAILED(hr)) { checkHR(hr, "put_WindowStyle failed"); cleanup();}

    RECT rc;
    GetClientRect((HWND)m_hwnd, &rc);
    hr = m_pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
    if (FAILED(hr)) { checkHR(hr, "SetWindowPosition failed"); cleanup();}

    hr = m_pMediaControl->Run();
    if (FAILED(hr)) { checkHR(hr, "Graph Run failed"); cleanup();}
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
    safeReleaseDirectShow(&m_pMediaControl);
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
    safeReleaseDirectShow(&m_pCameraControl);
    safeReleaseDirectShow(&m_pVideoProcAmp);

    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
}

void VideoCaptureProcessor::debugCameraControls()
{
    if (!m_pCameraControl) {
        qDebug() << "IAMCameraControl is NOT available";
        return;
    }

    struct Item {
        CameraControlProperty prop;
        const char* name;
    };

    Item props[] = {
        { CameraControl_Pan,      "Pan" },
        { CameraControl_Tilt,     "Tilt" },
        { CameraControl_Roll,     "Roll" },
        { CameraControl_Zoom,     "Zoom" },
        { CameraControl_Exposure, "Exposure" },
        { CameraControl_Exposure, "Exposure" },
        { CameraControl_Iris,     "Iris" },
        { CameraControl_Focus,    "Focus" }
    };

    for (auto &p : props) {
        long Min=0, Max=0, Step=0, Def=0, Flags=0;
        HRESULT hr = m_pCameraControl->GetRange(p.prop, &Min, &Max, &Step, &Def, &Flags);

        if (FAILED(hr)) {
            qDebug() << p.name << ": NOT supported (GetRange failed)";
            continue;
        }

        qDebug() << p.name << ": range=[" << Min << "," << Max
                 << "] step=" << Step
                 << " default=" << Def
                 << " flags=" << Flags;

        long cur=0, curFlags=0;
        hr = m_pCameraControl->Get(p.prop, &cur, &curFlags);
        if (SUCCEEDED(hr)) {
            qDebug() << "      Current =" << cur << " (flags=" << curFlags << ")";
        } else {
            qDebug() << "      Current: failed";
        }
    }
}

void VideoCaptureProcessor::debugVideoProcAmp()
{
    if (!m_pVideoProcAmp) {
        qDebug() << "IAMVideoProcAmp is NOT available";
        return;
    }

    struct Item {
        VideoProcAmpProperty prop;
        const char* name;
    };

    Item props[] = {
        { VideoProcAmp_Brightness,              "Brightness" },
        { VideoProcAmp_Contrast,                "Contrast" },
        { VideoProcAmp_Hue,                     "Hue" },
        { VideoProcAmp_Saturation,              "Saturation" },
        { VideoProcAmp_Sharpness,               "Sharpness" },
        { VideoProcAmp_Gamma,                   "Gamma" },
        { VideoProcAmp_ColorEnable,             "ColorEnable" },
        { VideoProcAmp_WhiteBalance,            "WhiteBalance" },
        { VideoProcAmp_BacklightCompensation,   "BacklightCompensation" },
        { VideoProcAmp_Gain,                    "Gain" }
    };

    qDebug() << "========== IAMVideoProcAmp ==========";

    for (auto &p : props) {

        long Min=0, Max=0, Step=0, Def=0, Flags=0;
        HRESULT hr = m_pVideoProcAmp->GetRange(p.prop, &Min, &Max, &Step, &Def, &Flags);

        if (FAILED(hr)) {
            qDebug() << p.name << ": NOT supported";
            continue;
        }

        qDebug() << p.name << ": range=[" << Min << "," << Max << "] step=" << Step
                 << " default=" << Def << " flags=" << Flags;

        long cur=0, curFlags=0;
        hr = m_pVideoProcAmp->Get(p.prop, &cur, &curFlags);
        if (SUCCEEDED(hr)) {
            qDebug() << "      Current =" << cur << " flags=" << curFlags;
        } else {
            qDebug() << "      Current: failed";
        }
    }
}


void VideoCaptureProcessor::debugAllCameraProps()
{
    qDebug() << "\n======= CAMERA PROPERTY DIAGNOSTICS =======\n";
    debugCameraControls();
    debugVideoProcAmp();
    qDebug() << "\n===========================================\n";
}

bool VideoCaptureProcessor::getVideoProcAmpCurrent(long prop, long& current, long& flags) {
    if (!m_pVideoProcAmp) return false;
    HRESULT hr = m_pVideoProcAmp->Get(prop, &current, &flags);
    return SUCCEEDED(hr);
}

bool VideoCaptureProcessor::setVideoProcAmpProperty(long prop, long value) {
    if (!m_pVideoProcAmp) return false;
    HRESULT hr = m_pVideoProcAmp->Set(prop, value, VideoProcAmp_Flags_Manual);
    return SUCCEEDED(hr);
}

bool VideoCaptureProcessor::getVideoProcAmpRange(long prop, long& min, long& max) {
    if (!m_pVideoProcAmp) return false;
    long step, defVal, flags;
    HRESULT hr = m_pVideoProcAmp->GetRange(prop, &min, &max, &step, &defVal, &flags);
    return SUCCEEDED(hr);
}
