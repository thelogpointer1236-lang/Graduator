#include "CameraSettings.h"
#include "dshow/VideoCaptureProcessor.h"
#include <dshow.h> // Для VideoProcAmp_*
#include <map>

namespace
{
    static const std::map<QString, int> settings = {
        { "brightness", static_cast<int>(VideoProcAmp_Brightness) },
        { "contrast",   static_cast<int>(VideoProcAmp_Contrast) },
        { "hue",        static_cast<int>(VideoProcAmp_Hue) },
        { "saturation", static_cast<int>(VideoProcAmp_Saturation) },
        { "sharpness",  static_cast<int>(VideoProcAmp_Sharpness) },
        { "gamma",      static_cast<int>(VideoProcAmp_Gamma) },
        { "backlight_compensation", static_cast<int>(VideoProcAmp_BacklightCompensation) }
    };
}


CameraSettings::CameraSettings() = default;

CameraSettings::~CameraSettings() = default;

void CameraSettings::setValue(const QString& key, const long rawValue)
{
    const auto it = settings.find(key);
    if (it == settings.end() || !video()) return;
    const int prop = it->second;
    video()->setVideoProcAmpProperty(prop, rawValue);
}

long CameraSettings::getValue(const QString& key, bool* ok) const
{
    const auto it = settings.find(key);
    if (it == settings.end() || !video()) {
        *ok = false;
        return 0;
    }
    const int prop = it->second;
    long current;
    long flags;
    if (!video_->getVideoProcAmpCurrent(prop, current, flags)) {
        *ok = false;
        return LONG_MAX; // Ошибка получения значения
    }
    *ok = true;
    return current;
}

std::pair<int, int> CameraSettings::getKeyRange(const QString& key, bool* ok) const
{
    const auto it = settings.find(key);
    if (it == settings.end() || !video() || !video_) {
        *ok = false;
         return std::make_pair(0, 0);
    }
    const int prop = it->second;
    long min, max;
    if (!video_->getVideoProcAmpRange(prop, min, max)) {
        if (ok) *ok = false;
        return std::make_pair(0, 0);
    }
    *ok = true;
    return std::make_pair((int)min, (int)max);
}

QStringList CameraSettings::keys() {
    QStringList result;
    result.reserve(settings.size());   // если settings — map, size() O(1)

    std::transform(settings.begin(), settings.end(),
                   std::back_inserter(result),
                   [](auto const& kv) { return kv.first; });

    return result;
}

void CameraSettings::setVideoCapProcessor(VideoCaptureProcessor *videoProcessor)
{
    video_ = videoProcessor;
}

VideoCaptureProcessor * CameraSettings::video() const { return video_; }
