#include "CameraSettings.h"
#include "dshow/VideoCaptureProcessor.h"
#include <dshow.h> // Для VideoProcAmp_*

CameraSettings::CameraSettings(VideoCaptureProcessor* videoProcessor) {
    // settings_[ключ] = { videoProcAmpPropertyId, текущее значение }
    settings_.emplace("brightness", QPair<int, long>(VideoProcAmp_Brightness, 0));
    settings_.emplace("contrast", QPair<int, long>(VideoProcAmp_Contrast, 0));
    settings_.emplace("hue", QPair<int, long>(VideoProcAmp_Hue, 0));
    settings_.emplace("saturation", QPair<int, long>(VideoProcAmp_Saturation, 0));
    settings_.emplace("sharpness", QPair<int, long>(VideoProcAmp_Sharpness, 0));
    settings_.emplace("gamma", QPair<int, long>(VideoProcAmp_Gamma, 0));
    settings_.emplace("backlight_compensation", QPair<int, long>(VideoProcAmp_BacklightCompensation, 0));

    // Можно сразу загрузить начальные значения, если требуется
    for (auto& kv : settings_) {
        long min, max;
        videoProcessor->getVideoProcAmpRange(kv.second.first, min, max);
        // Ничего не ставим, но можем учитывать диапазоны при необходимости
    }
}

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
        return 0;
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

void CameraSettings::getAvailableKeys(QVector<QString>& keys) const {
    keys.clear();
    for (auto& kv : settings_) {
        keys.push_back(kv.first);
    }
}
