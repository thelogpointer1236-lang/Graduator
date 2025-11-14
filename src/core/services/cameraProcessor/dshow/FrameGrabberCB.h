//
// Created by User on 28.08.2025.
//
#ifndef FRAMEGRABBERCB_H
#define FRAMEGRABBERCB_H
#include <dshow.h>
#include <windows.h>
#include "image/RGBImage.h"
#include <wrl/client.h>
// QEDIT.H -
// Блядский код, хз кто его написал, и хз почему его не включили в SDK.
// Хули я сам должен его искать и включать?
#include "QEDIT.H"
#include <QElapsedTimer>
#include <functional>
#include <mutex>
#include <QVector>
#include <QObject>
#pragma comment(lib, "strmiids.lib")
using namespace Microsoft::WRL;
class Anglemeter;
class FrameGrabberCB final : public QObject, public ISampleGrabberCB {
    Q_OBJECT;
public:
    explicit FrameGrabberCB(qint32 camIdx, QObject *parent = nullptr);
    ~FrameGrabberCB() override;
    // IUnknown methods
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    // ISampleGrabberCB methods
    STDMETHOD (SampleCB)(double SampleTime, IMediaSample *pSample) override;
    STDMETHOD (BufferCB)(double SampleTime, BYTE *pBuffer, long BufferLen) override;
    signals:
    
    void angleReady(qint32 idx, qreal t, qreal a);
public:
    static inline auto s_aimColor = RGBPixel(0, 0, 0);
    static inline bool s_isDrawAim = false;
    static inline bool s_isPolling = false;
    static inline bool s_isDrawAngle = true;
private:
    LONG m_refCount;
    qint32 m_camIdx;
};
#endif //FRAMEGRABBERCB_H