#include "FrameGrabberCB.h"
#include "../anglemeters/scan_anglemeter.h"
#include "core/services/ServiceLocator.h"
#include <iostream>
FrameGrabberCB::FrameGrabberCB(const qint32 camIdx, QObject *parent)
    : QObject(parent)
      , m_camIdx(camIdx)
      , m_refCount(1) {
}
FrameGrabberCB::~FrameGrabberCB() {
}
HRESULT FrameGrabberCB::QueryInterface(const IID &riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
        *ppv = dynamic_cast<ISampleGrabberCB *>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}
ULONG FrameGrabberCB::AddRef() {
    return InterlockedIncrement(&m_refCount);
}
ULONG FrameGrabberCB::Release() {
    const LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) {
        delete this;
    }
    return count;
}
HRESULT FrameGrabberCB::SampleCB(double SampleTime, IMediaSample *pSample) {
    return S_OK;
}
HRESULT FrameGrabberCB::BufferCB(double SampleTime, BYTE *pBuffer, const long BufferLen) {
    if (!pBuffer) return E_POINTER;

    RGBImage image(pBuffer, BufferLen); // no copy - just a wrapper
    if (s_isPolling) {
        const BYTE *pixelData = pBuffer;
        const qreal timestampSec = ServiceLocator::instance().graduationService()->getElapsedTimeSeconds();
        float angleDeg; {
            angleDeg = scanArrowAngle(pixelData, image.getWidth(), image.getHeight());
            if (angleDeg <= 0.0) return S_OK;
            angleDeg = std::fmodf(180.f - angleDeg, 360.0f);
            if (angleDeg < 0.0f) {
                angleDeg += 360.0f;
            }
            if (s_isDrawAngle) {
                image.drawFloatNumber(40, 40 + 0, angleDeg, 2, RGBPixel(255, 0, 0), 2);
            }
        }
        emit angleReady(m_camIdx + 1, timestampSec, static_cast<qreal>(angleDeg));
    }
    if (s_isDrawAim) {
        constexpr int r = 50;
        for (int i = 0; i < 4; i++)
            image.drawCircle(image.getWidth() / 2, image.getHeight() / 2, r - 3 + i, s_aimColor, 1);
        for (int i = 0; i < 3; i++)
            image.drawCircle(image.getWidth() / 2, image.getHeight() / 2, r + i,
                             RGBPixel(255 - s_aimColor.r, 255 - s_aimColor.g, 255 - s_aimColor.b), 1);
        for (int i = 0; i < 4; i++)
            image.drawCircle(image.getWidth() / 2, image.getHeight() / 2, r + 4 + i, s_aimColor, 1);
    }
    return S_OK;
}
