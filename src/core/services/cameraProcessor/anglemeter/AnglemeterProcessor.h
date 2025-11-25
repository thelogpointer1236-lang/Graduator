#ifndef GRADUATOR_ANGLEMETERPROCESSOR_H
#define GRADUATOR_ANGLEMETERPROCESSOR_H
#include <QObject>
#include <atomic>
#include "cast_anglemeter.h"


class AnglemeterProcessor final : public QObject {
    Q_OBJECT
public:
    AnglemeterProcessor();
    ~AnglemeterProcessor() override;
    void setImageSize(qint32 width, qint32 height);
    void setAngleTransformation(float (*func_ptr)(float));

    void enqueueImage(qint32 cameraIdx, qreal time, quint8* imgData);
    qint32 queueSize() const;

private slots:
    void processImage(qint32 cameraIdx, qreal time, quint8 *imgData);

signals:
    void angleMeasured(qint32 cameraIdx, qreal time, qreal angle);

private:
    anglemeter_t* m_am;
    std::atomic<qint32> m_queueSize;
};

#endif //GRADUATOR_ANGLEMETERPROCESSOR_H
