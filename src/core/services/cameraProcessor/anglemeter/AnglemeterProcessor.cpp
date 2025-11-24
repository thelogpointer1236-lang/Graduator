#include "AnglemeterProcessor.h"
#include <QThread>

AnglemeterProcessor::AnglemeterProcessor() {
    anglemeterCreate(&m_am);
}

AnglemeterProcessor::~AnglemeterProcessor() {
    anglemeterDestroy(m_am);
}

qint32 AnglemeterProcessor::queueSize() const { return m_queueSize.load(); }

void AnglemeterProcessor::setImageSize(qint32 width, qint32 height) {
    anglemeterSetImageSize(m_am, static_cast<int>(width), static_cast<int>(height));
}

void AnglemeterProcessor::enqueueImage(qint32 cameraIdx, qreal time, quint8* imgData) {
    QMetaObject::invokeMethod(this, "processImage",
                              Qt::QueuedConnection,
                              Q_ARG(qint32, cameraIdx),
                              Q_ARG(qreal, time),
                              Q_ARG(quint8*, imgData));
    ++m_queueSize;
}

void AnglemeterProcessor::processImage(qint32 cameraIdx, qreal time, quint8* imgData) {
    float angle;
    if (anglemeterGetArrowAngle(m_am, imgData, &angle)) {
        emit angleMeasured(time, cameraIdx, static_cast<qreal>(angle));
    }
    anglemeterFreeImage(imgData);
    --m_queueSize;
}
