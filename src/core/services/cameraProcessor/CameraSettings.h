#ifndef GRADUATOR_CAMERASETTINGS_H
#define GRADUATOR_CAMERASETTINGS_H
#include <map>
#include <QString>
#include <QPair>
#include <QStringList>

class VideoCaptureProcessor;
class CameraSettings {
public:
    explicit CameraSettings();
    ~CameraSettings();

    void setValue(const QString& key, long rawValue);
    long getValue(const QString& key, bool* ok = nullptr) const;
    std::pair<int, int> getKeyRange(const QString& key, bool* ok = nullptr) const;

    static QStringList keys();
    void setVideoCapProcessor(VideoCaptureProcessor* videoProcessor);
    VideoCaptureProcessor *video() const;

private:
    VideoCaptureProcessor* video_ = nullptr;
};


#endif //GRADUATOR_CAMERASETTINGS_H
