#ifndef GRADUATOR_CAMERASETTINGS_H
#define GRADUATOR_CAMERASETTINGS_H
#include <map>
#include <QString>
#include <QPair>

class VideoCaptureProcessor;
class CameraSettings {
public:
    explicit CameraSettings(VideoCaptureProcessor* videoProcessor);
    ~CameraSettings();

    void setValue(const QString& key, long rawValue);
    long getValue(const QString& key, bool* ok = nullptr) const;
    void getKeyRange(const QString& key, long& min, long& max, bool* ok = nullptr) const;
    void getAvailableKeys(QVector<QString>& keys) const;

private:
    std::map<QString, QPair<int, long>> settings_;
    VideoCaptureProcessor* video_;
};


#endif //GRADUATOR_CAMERASETTINGS_H
