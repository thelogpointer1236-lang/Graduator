#ifndef GRADUATOR_PRESSURESENSOR_H
#define GRADUATOR_PRESSURESENSOR_H
#include <QObject>
#include <QVector>
#include "core/types/Pressure.h"
class PressureSensor final : public QObject {
    Q_OBJECT
public:
    explicit PressureSensor(QObject *parent = nullptr);
    ~PressureSensor();
    Pressure getLastPressure() const;
    bool openCOM(const QString &comPort, QString &error);
    bool isRunning() const;
    void stop();
public
    slots:
    Q_INVOKABLE
    void start();
    signals:
    
    void pressureMeasured(qreal t, Pressure p);
private:
    void closeCOM();
    bool pollOnce(QString &error);
    void *m_hSerial;
    QString m_comPort;
    std::atomic<bool> m_isRunning = false;
    std::atomic<bool> m_aboutToStop = false;
    QByteArray m_requestBytes;
    QVector<int> m_pressureByteIndices; // Индексы байтов в ответе, где лежат данные давления (4 байта float)
    int m_unitByteIndex; // Индекс байта в ответе, где кодируется единица измерения
    int m_responseLength; // Ожидаемая длина ответа от устройства (байты)
    QByteArray m_responseBuffer; // Буфер для фактического ответа от устройства
    Pressure m_lastPressure;
};
#endif //GRADUATOR_PRESSURESENSOR_H