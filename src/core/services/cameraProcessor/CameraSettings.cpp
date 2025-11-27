#include "CameraSettings.h"
#include "dshow/VideoCaptureProcessor.h"
#include <dshow.h> // Для VideoProcAmp_*

CameraSettings::CameraSettings() = default;

CameraSettings::~CameraSettings() = default;

void CameraSettings::setValue(const QString& key, long rawValue) {
    auto it = settings_.find(key);
    if (it == settings_.end()) return;

    int prop = it->second.first;
    it->second.second = rawValue;

    // Передача в VideoCaptureProcessor
    // videoProcessor через конструктор — добавь поле:
    // VideoCaptureProcessor* video_;
    video_->setVideoProcAmpProperty(prop, rawValue);
}

long CameraSettings::getValue(const QString& key, bool* ok) const {
    auto it = settings_.find(key);
    if (it == settings_.end()) {
        if (ok) *ok = false;
        long prop = settings_.at(key).second;
        long current;
        long flags;
        if (!video_->getVideoProcAmpCurrent(prop, current, flags)) {
            return -9999; // Ошибка получения значения
        }
        return current;
    }
    if (ok) *ok = true;
    return it->second.second;
}

void CameraSettings::getKeyRange(const QString& key, long& min, long& max, bool* ok) const {
    auto it = settings_.find(key);
    if (it == settings_.end()) {
        if (ok) *ok = false;
        return;
    }

    int prop = it->second.first;
    if (video_->getVideoProcAmpRange(prop, min, max)) {
        if (ok) *ok = true;
    } else {
        if (ok) *ok = false;
    }
}

QVector<QString> CameraSettings::keys() const {
    QVector<QString> keys_;
    keys_.clear();
    for (auto& kv : settings_) {
        keys_.push_back(kv.first);
    }
    return keys_;
}

void CameraSettings::setVideoCapProcessor(VideoCaptureProcessor *videoProcessor) {
    video_ = videoProcessor;

    // Список поддерживаемых VideoProcAmp свойств
    settings_.clear();
    settings_.emplace("brightness", QPair<int, long>(VideoProcAmp_Brightness, 0));
    settings_.emplace("contrast", QPair<int, long>(VideoProcAmp_Contrast, 0));
    settings_.emplace("hue", QPair<int, long>(VideoProcAmp_Hue, 0));
    settings_.emplace("saturation", QPair<int, long>(VideoProcAmp_Saturation, 0));
    settings_.emplace("sharpness", QPair<int, long>(VideoProcAmp_Sharpness, 0));
    settings_.emplace("gamma", QPair<int, long>(VideoProcAmp_Gamma, 0));
    settings_.emplace("backlight_compensation", QPair<int, long>(VideoProcAmp_BacklightCompensation, 0));

    // Получение диапазонов и текущих значений
    for (auto& kv : settings_) {
        int prop = kv.second.first;

        long min, max;
        videoProcessor->getVideoProcAmpRange(prop, min, max);

        long current = 0;
        long flags = 0;

        if (videoProcessor->getVideoProcAmpCurrent(prop, current, flags)) {
            kv.second.second = current;   // записываем актуальное значение камеры
        }
    }
}

VideoCaptureProcessor * CameraSettings::video() const { return video_; }
