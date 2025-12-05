#include "AnglemeterProcessor.h"
#include <QThread>
#include <QDebug>

AnglemeterProcessor::AnglemeterProcessor() {
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
