#include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"
#include "dshow/FrameGrabberCB.h"
#include "CameraProcessor.h"
#include "defines.h"
#include "core/services/ServiceLocator.h"
#include <QThread>
#include <QDebug>
#include <algorithm>

CameraProcessor::CameraProcessor(int anglemeterThreadsCount, int imgWidth, int imgHeight, QObject *parent) : QObject(parent) {
    m_videoProcessors.reserve(8);
    for (int i = 0; i < anglemeterThreadsCount; ++i) {
        auto* anglemeterProcessor = new AnglemeterProcessor();
        anglemeterProcessor->setImageSize(imgWidth, imgHeight);
        anglemeterProcessor->setAngleTransformation([] (const float angleDeg) -> float {
            float transformedAngle = std::fmodf(180.0f - angleDeg, 360.0f);
            if (transformedAngle < 0.0f) transformedAngle += 360.0f;
            return transformedAngle;

        });
        auto* thread = new QThread();
        anglemeterProcessor->moveToThread(thread);
        thread->start();
        m_anglemeterProcessors.emplace_back(anglemeterProcessor);
        m_anglemeterThreads.emplace_back(thread);
        connect(anglemeterProcessor, &AnglemeterProcessor::angleMeasured,
            this, &CameraProcessor::angleMeasured);
    }
}

CameraProcessor::~CameraProcessor() {
    for (auto* anglemeterProcessor : m_anglemeterProcessors) {
        anglemeterProcessor->deleteLater();
    }
    for (auto* thread : m_anglemeterThreads) {
        thread->quit();
        thread->wait();
        delete thread;
    }
}

void CameraProcessor::setCameraString(const QString &cameraStr) {
    const int cameraLimit = availableCameraCount();
    auto cleanAndUnique = [cameraLimit](const QString &input) -> QString {
        QString digits;
        for (QChar c: input) {
            if (c.isDigit() && c != '0' && c.digitValue() <= cameraLimit && !digits.contains(c))
                digits.append(c);
        }
        return digits;
    };
    const QString camClean = cleanAndUnique(cameraStr);
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_CAMERA_STR, camClean);
    emit cameraStrChanged(camClean);
}

void CameraProcessor::setCameraIndices(std::vector<qint32> indices) {
    QString cameraStr;
    const int cameraLimit = availableCameraCount();
    std::sort(indices.begin(), indices.end());
    for (qint32 idx: indices) {
        if (idx > 0 && idx <= cameraLimit) {
            cameraStr.append(QString::number(idx));
        }
    }
    setCameraString(cameraStr);
}

VideoCaptureProcessor* CameraProcessor::createVideoProcessor(QObject *parent) {
    auto* vp = new VideoCaptureProcessor(this);
    connect(vp, &VideoCaptureProcessor::imageCaptured,
        this, &CameraProcessor::enqueueImage, Qt::DirectConnection);
    m_videoProcessors.emplace_back(vp);
    return vp;
}

const std::vector<VideoCaptureProcessor*>& CameraProcessor::videoProcessors() {
    return m_videoProcessors;
}

void CameraProcessor::clearVideoProcessors() {
    for (auto* vp : m_videoProcessors) {
        delete vp;
    }
    m_videoProcessors.clear();
}

int CameraProcessor::availableCameraCount() {
    return VideoCaptureProcessor::getCameraCount();
}

void CameraProcessor::enableDrawingCrosshair(bool enabled) {
    FrameGrabberCB::s_isDrawingAim = enabled;
}

QString CameraProcessor::cameraStr() {
    return ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_CURRENT_CAMERA_STR, "");
}

QString CameraProcessor::sysCameraStr() {
    return ServiceLocator::instance().configManager()->get<QString>(CFG_KEY_SYS_CAMERA_STR, "");
}

namespace {
    std::vector<qint32> extractDigits(const QString &input) {
        std::vector<qint32> result;
        result.reserve(input.size());
        for (QChar c: input) {
            if (c.isDigit()) {
                result.push_back(c.digitValue());
            }
        }
        return result;
    }
}

std::vector<qint32> CameraProcessor::cameraIndices() {
    return extractDigits(cameraStr());
}

std::vector<qint32> CameraProcessor::sysCameraIndices() {
    return extractDigits(sysCameraStr());
}

void CameraProcessor::enqueueImage(qint32 cameraIdx, qreal time, quint8 *imgData) {
    if (m_anglemeterProcessors.empty()) return;

    const auto it = std::min_element(
        m_anglemeterProcessors.begin(), m_anglemeterProcessors.end(),
        [](const auto &a, const auto &b){ return a->queueSize() < b->queueSize(); }
    );

    for (auto& anglemeterProcessor : m_anglemeterProcessors) {
        qDebug() << "queue size:" << anglemeterProcessor->queueSize();
    }

    (*it)->enqueueImage(cameraIdx, time, imgData);
}
