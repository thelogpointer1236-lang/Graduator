#ifndef GRADUATOR_CAMERAGRIDWIDGET_H

#define GRADUATOR_CAMERAGRIDWIDGET_H

#include "AspectRatioLabel.h"

#include <QWidget>
#include <QGridLayout>

#include <vector>

class CameraGridWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraGridWidget(QWidget *parent = nullptr);

private slots:
    void onCameraStrChanged(const QString &newCameraStr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupLayout();
    void setupConnections();
    void createCameraLabels();
    AspectRatioLabel *createCameraLabel(int index);
    void clearCameraLabels();
    void refresh();
    void updateLayout();
    QSize getCameraWindowSize() const;
    void rebuildVideoProcessors(int camCount, const std::vector<int> &userIndices,
                                 const std::vector<int> &sysIndices);
    void clearVideoProcessors();

    QGridLayout *gridLayout = nullptr;
    std::vector<AspectRatioLabel *> cameraLabels;
};

#endif // GRADUATOR_CAMERAGRIDWIDGET_H
