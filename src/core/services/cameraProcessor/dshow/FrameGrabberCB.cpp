#include "FrameGrabberCB.h"
#include "../anglemeter/cast_anglemeter.h"
#include "core/services/ServiceLocator.h"
#include <iostream>
#include <QDebug>

FrameGrabberCB::FrameGrabberCB(qint32 camIdx, qint32 imgWidth, qint32 imgHeight, QObject *parent)
    : QObject(parent)
      , m_camIdx(camIdx)
      , m_refCount(1)
{
    anglemeterCreate(&m_am);
    anglemeterSetImageSize(m_am, 640, 480);
    anglemeterSetAngleTransformation(m_am, [] (const float angleDeg) -> float {
        float transformedAngle = std::fmodf(180.0f - angleDeg, 360.0f);
        if (transformedAngle < 0.0f) transformedAngle += 360.0f;
        return transformedAngle;

    });
}

FrameGrabberCB::~FrameGrabberCB() {
    anglemeterDestroy(m_am);
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

    int capIdx = 0;

    // -------------------------
    // Захватываем изображение
    // -------------------------
    if (s_isCapturing) {
        if (capIdx % s_capRate == 0) {
            const BYTE *pixelData = pBuffer;
            const qreal timestampSec = ServiceLocator::instance().graduationService()->getElapsedTimeSeconds();

            float angle;
            anglemeterRestoreState(m_am);
            bool ok = anglemeterGetArrowAngle(m_am, pixelData, &angle);

            if (ok) {
                // отнимаем 60 мс, так как задержка захвата кадра
                emit angleMeasured(m_camIdx, timestampSec - 0.060, angle);
            }

        }
        capIdx++;
    }

    // -------------------------
    // Рисуем прицел
    // -------------------------
    if (s_aimIsVisible) {
        RGBImage image(pBuffer, BufferLen);

        const int cx = image.getWidth() / 2;
        const int cy = image.getHeight() / 2;

        struct Ring { int start; int count; RGBPixel color; };

        Ring rings[] = {
            { s_aimRadius - 3, 4, s_aimColor },
            { s_aimRadius,     3, RGBPixel(255 - s_aimColor.r, 255 - s_aimColor.g, 255 - s_aimColor.b) },
            { s_aimRadius + 4, 4, s_aimColor }
        };

        for (const auto& r : rings)
            for (int i = 0; i < r.count; ++i)
                image.drawCircle(cx, cy, r.start + i, r.color, 1);
    }

    return S_OK;
}
