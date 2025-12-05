#include "core/services/cameraProcessor/dshow/VideoCaptureProcessor.h"
#include "dshow/FrameGrabberCB.h"
#include "CameraProcessor.h"
#include "defines.h"
#include "core/services/ServiceLocator.h"
#include <QThread>
#include <QDebug>
#include <QColor>
#include <algorithm>

namespace {
    constexpr qreal kMaxExpectedAngleDeg = 340.0;
}


CameraProcessor::CameraProcessor(int anglemeterThreadsCount, int imgWidth, int imgHeight, QObject *parent) : QObject(parent) {
    m_cameras.reserve(8);
    createAnglemeterWorkers(anglemeterThreadsCount, imgWidth, imgHeight);
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
    m_lastAngles.clear();
    m_angleOvershootAcknowledged.clear();
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

Camera& CameraProcessor::openCamera(void *hwnd, int cameraIndex) {
    m_cameras.emplace_back();
    Camera& cam = m_cameras.back();
    cam.init(hwnd, cameraIndex);
    connect(cam.video(), &VideoCaptureProcessor::imageCaptured,
    this, &CameraProcessor::enqueueImage, Qt::DirectConnection);
    return cam;
}

std::vector<Camera>& CameraProcessor::cameras() {
    return m_cameras;
}

void CameraProcessor::closeCameras() {
    m_cameras.clear();
}

void CameraProcessor::setCapRate(int rate) {
    FrameGrabberCB::s_capRate = rate;
}

void CameraProcessor::restoreDefaultCapRate() {
    FrameGrabberCB::s_capRate = 4;
}

qreal CameraProcessor::lastAngleForCamera(qint32 cameraIdx) const {
    auto it = m_lastAngles.find(cameraIdx);
    if (it != m_lastAngles.end()) {
        return it->second;
    }
    return qreal{0};
}

int CameraProcessor::availableCameraCount() {
    return VideoCaptureProcessor::getCameraCount();
}

void CameraProcessor::setAimEnabled(bool enabled) {
    FrameGrabberCB::s_aimIsVisible = enabled;
}

void CameraProcessor::setCapturingRate(int rate) {
}

void CameraProcessor::setAimColor(const QColor &color) {
    FrameGrabberCB::s_aimColor = RGBPixel(
        static_cast<quint8>(color.red()),
        static_cast<quint8>(color.green()),
        static_cast<quint8>(color.blue())
    );
}

void CameraProcessor::setAimRadius(int radius) {
    FrameGrabberCB::s_aimRadius = radius;
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

void CameraProcessor::emitCamerasChanged() {
    emit camerasChanged();
}

void CameraProcessor::enqueueImage(qint32 cameraIdx, qreal time, quint8 *imgData) {
    if (m_anglemeterProcessors.empty()) return;

    const auto it = std::min_element(
        m_anglemeterProcessors.begin(), m_anglemeterProcessors.end(),
        [](const auto &a, const auto &b){ return a->queueSize() < b->queueSize(); }
    );

    (*it)->enqueueImage(cameraIdx, time, imgData);
}

void CameraProcessor::onAngleMeasured(qint32 idx, qreal time, qreal angle) {
    const bool angleExceeded = angle > kMaxExpectedAngleDeg;
    if (angleExceeded && m_angleOvershootAcknowledged.find(idx) == m_angleOvershootAcknowledged.end()) {
        if (auto *dialog = ServiceLocator::instance().userDialogService()) {
            const QString yesOption = tr("Yes");
            const QString noOption = tr("No");

            QString response = dialog->requestUserInput(
                tr("Gauge angle exceeded"),
                tr("The gauge pointer angle is higher than expected. Continue?"),
                {yesOption, noOption}
            );

            if (response == noOption) {
                if (auto *graduationService = ServiceLocator::instance().graduationService()) {
                    graduationService->interrupt();
                }
                return;
            }
        }
        m_angleOvershootAcknowledged.insert(idx);
    }

    if (!angleExceeded) {
        m_angleOvershootAcknowledged.erase(idx);
    }

    emit angleMeasured(idx, time, angle);
    m_lastAngles[idx] = angle;
}

void CameraProcessor::createAnglemeterWorkers(int anglemeterThreadsCount, int imgWidth, int imgHeight) {
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
                this, &CameraProcessor::onAngleMeasured);

        // connect(anglemeterProcessor, &AnglemeterProcessor::angleMeasured,
        //     this, &CameraProcessor::angleMeasured);
        // connect(this, &CameraProcessor::angleMeasured, this, &CameraProcessor::onAngleMeasured);
    }
}
