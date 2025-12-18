#ifndef GRADUATOR_CAMERASETTINGSTAB_H
#define GRADUATOR_CAMERASETTINGSTAB_H

#include <QWidget>

class QVBoxLayout;
class QGroupBox;
class QSlider;
class QLabel;

class CameraSettingsTab final : public QWidget {
    Q_OBJECT
public:
    explicit CameraSettingsTab(QWidget *parent = nullptr);

private slots:
    void rebuildCameraSettings();
    void onCamerasChanged();
    void onRecordToggled(bool checked);

private:
    void setupUi();
    void clearCameraGroups();
    QGroupBox *createCameraGroup(int cameraIndex);
    QWidget *createSettingRow(int cameraIndex, const QString &key, long min, long max, long current);
    void applySetting(int cameraIndex, const QString &key, int value);

private:
    QVBoxLayout *camerasLayout_ = nullptr;
    bool settingsLoadedFromFile_ = false;
};

#endif // GRADUATOR_CAMERASETTINGSTAB_H
