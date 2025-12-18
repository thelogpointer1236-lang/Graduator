#include "FrameGrabberCB.h"
#include "../anglemeter/cast_anglemeter.h"
#include "core/services/ServiceLocator.h"
#include <iostream>
#include <QDebug>
#include <QImage>
#include <QDir>
#include <QAtomicInteger>



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

    static bool dirReady = false;
    if (!dirReady) {
        QDir().mkpath("frames");
        dirReady = true;
    }

    const int width  = 640;
    const int height = 480;

    const quint64 frameIdx = m_frameCounter.fetchAndAddRelaxed(1);

    // -------------------------
    // Захватываем изображение
    // -------------------------
    if (s_isCapturing) {
        if (true) {
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
    }

    if (frameIdx % 5 == 0 && s_isFramesRecording) {
        QImage img(
            pBuffer,
            width,
            height,
            width * 3,
            QImage::Format_BGR888
        );

        const QString fileName = QString("frames/cam%1_frame%2.png")
            .arg(m_camIdx)
            .arg(frameIdx, 6, 10, QChar('0'));

        img.save(fileName, "PNG");
    }




    // -------------------------
    // Рисуем границы (красные крестики)
    // -------------------------
    if (s_isEdgeVisible) {
        const int bytesPerPixel = 3;
        const int stride = width * bytesPerPixel;

        auto drawCross = [&](int x, int y) {
            auto drawPixel = [&](int px, int py) {
                if (px < 0 || px >= width || py < 0 || py >= height)
                    return;

                BYTE* pixel = pBuffer + py * stride + px * bytesPerPixel;
                pixel[0] = 0;    // B
                pixel[1] = 0;    // G
                pixel[2] = 255;  // R
            };

            drawPixel(x,     y);
            drawPixel(x - 1, y);
            drawPixel(x + 1, y);
            drawPixel(x, y - 1);
            drawPixel(x, y + 1);
            drawPixel(x - 1, y + 1);
            drawPixel(x + 1, y - 1);
            drawPixel(x + 1, y + 1);
            drawPixel(x - 1, y - 1);
        };

        for (const auto& p : m_am->points_1)
            drawCross(p.x, p.y);

        for (const auto& p : m_am->points_2)
            drawCross(p.x, p.y);
    }



    // -------------------------
    // Рисуем прицел
    // -------------------------
    if (s_isAimVisible) {
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
