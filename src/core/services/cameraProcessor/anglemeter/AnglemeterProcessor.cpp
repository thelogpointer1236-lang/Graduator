#include "AnglemeterProcessor.h"
#include <QThread>
#include <QDebug>
#include <cmath>

#ifdef USE_STUB_IMPLEMENTATIONS

AnglemeterProcessor::AnglemeterProcessor() : m_am(nullptr), m_queueSize(0) {}

AnglemeterProcessor::~AnglemeterProcessor() = default;

void AnglemeterProcessor::setImageSize(qint32 width, qint32 height) {
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void AnglemeterProcessor::setAngleTransformation(float(*func_ptr)(float)) {
    Q_UNUSED(func_ptr);
}

void AnglemeterProcessor::enqueueImage(qint32 cameraIdx, qreal time, const quint8* imgData) {
    QMetaObject::invokeMethod(this, "processImage",
                              Qt::QueuedConnection,
                              Q_ARG(qint32, cameraIdx),
                              Q_ARG(qreal, time),
                              Q_ARG(const quint8*, imgData));
    ++m_queueSize;
}

qint32 AnglemeterProcessor::queueSize() const {
    return m_queueSize.load();
}

void AnglemeterProcessor::processImage(qint32 cameraIdx, qreal time, const quint8* imgData) {
    Q_UNUSED(imgData);

    const double minAngle = 30.0;
    const double maxAngle = 310.0;
    const double period = 30.0;
    const double halfPeriod = period / 2.0;
    const double amplitude = maxAngle - minAngle;

    const double phase = std::fmod(static_cast<double>(time), period);
    double angle = minAngle;

    if (phase < halfPeriod) {
        angle = minAngle + (phase / halfPeriod) * amplitude;
    } else {
        angle = maxAngle - ((phase - halfPeriod) / halfPeriod) * amplitude;
    }

    emit angleMeasured(cameraIdx, time, static_cast<qreal>(angle));
    --m_queueSize;
}

#else

AnglemeterProcessor::AnglemeterProcessor() : m_am(nullptr), m_queueSize(0) {
    anglemeterCreate(&m_am);
    if (!anglemeterIsImageBufferCreated())
        anglemeterCreateImageBuffer(640, 480);
}

AnglemeterProcessor::~AnglemeterProcessor() {
    anglemeterDestroy(m_am);
}

void AnglemeterProcessor::setImageSize(qint32 width, qint32 height) {
    anglemeterSetImageSize(m_am, static_cast<int>(width), static_cast<int>(height));
}

void AnglemeterProcessor::setAngleTransformation(float(*func_ptr)(float)) {
    anglemeterSetAngleTransformation(m_am, func_ptr);
}

void AnglemeterProcessor::enqueueImage(qint32 cameraIdx, qreal time, const quint8* imgData) {
    QMetaObject::invokeMethod(this, "processImage",
                              Qt::QueuedConnection,
                              Q_ARG(qint32, cameraIdx),
                              Q_ARG(qreal, time),
                              Q_ARG(const quint8*, imgData));
    ++m_queueSize;
}

qint32 AnglemeterProcessor::queueSize() const {
    return m_queueSize.load();
}

void AnglemeterProcessor::processImage(qint32 cameraIdx, qreal time, const quint8* imgData) {
    float angle;
    if (anglemeterGetArrowAngle(m_am, imgData, &angle)) {
        // qDebug() << "angle measured:" << angle;
        emit angleMeasured(cameraIdx, time, static_cast<qreal>(angle));
    }
    anglemeterFreeImage(imgData);
    --m_queueSize;
}

#endif