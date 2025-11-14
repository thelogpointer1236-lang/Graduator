#ifndef GRADUATOR_G540CONTROLLER_H
#define GRADUATOR_G540CONTROLLER_H
#include <QObject>
#include <QElapsedTimer>
#include "LPTDriver.h"
enum class G540Direction {
    Forward,
    Backward
};
enum class G540FlapsState {
    Unknown,
    CloseBoth,
    OpenInput,
    OpenOutput
};
class G540Driver final : public QObject {
    Q_OBJECT
public:
    explicit G540Driver(QObject *parent = nullptr);
    ~G540Driver() override;
    void setDirection(G540Direction direction);
    G540Direction direction() const;
    void setDangerousPressure(qreal dangerousPressure);
    void updatePressure(qreal pressure);
    void setFrequency(int frequency);
    quint32 frequency() const;
    quint32 impulsesCount() const;
    quint32 maxFrequency() const;
    quint32 minFrequency() const;
    bool isAnyLimitTriggered() const;
    bool isStartLimitTriggered() const;
    bool isEndLimitTriggered() const;
    void setFlapsState(G540FlapsState flap);
    G540FlapsState flapsState() const;
    qreal timeSinceStartSec() const;
    bool isReadyToStart(QString &err) const;
    void stop();
    bool isRunning() const;
public
    slots:
    Q_INVOKABLE
    void start();
private:
    static void delayMicroseconds(long long target);
    quint8 state() const;
    void writePortByte(quint8 byte) const;
    quint8 readPortByte() const;
    void writeOffsetPortByte(quint16 offset, quint8 byte) const;
    quint8 readOffsetPortByte(quint16 offset) const;
    quint8 byteStartLimitSwitch() const;
    quint8 byteEndLimitSwitch() const;
private:
    std::unique_ptr<LPTDriver> m_LPTDriver;
    G540Direction m_direction = G540Direction::Forward;
    G540FlapsState m_flapsState = G540FlapsState::Unknown;
    mutable bool m_startLimitTriggered = false;
    mutable bool m_endLimitTriggered = false;
    std::atomic<bool> m_isRunning = false;
    std::atomic<bool> m_aboutToStop = false;
    QString m_lastError;
    qreal m_dangerousPressure = 99999999.0;
    qreal m_currentPressure = 0;
    qint64 m_lastTimeMsDangP = 0;
    QElapsedTimer m_elapsedTimerDangP;
    // Следит сколько времени прошло с последнего момента задания частоты
    qint64 m_lastTimeMs = 0;
    QElapsedTimer m_elapsedTimer;
    quint32 m_impulsesCount = 0;
    quint32 m_frequency = 0;
    quint32 m_halfDelayMcs = 0;
    quint8 m_byte1 = 0;
    quint8 m_byte2 = 0;
    // Параметры:
    // 1. Адрес порта LPT (обычно 0x378 для первого порта)
    quint16 m_portAddress = 0; // адрес LPT-порта
    // 3. Байт для закрытия обеих клапанов
    quint8 m_byteCloseBothFlaps = 0; // байт для закрытия обеих клапанов
    // 4. Байт для открытия входного клапана
    quint8 m_byteOpenInputFlap = 0; // байт для открытия входного клапана
    // 5. Байт для открытия выходного клапана
    quint8 m_byteOpenOutputFlap = 0; // байт для открытия выходного клапана
    // 6. Бит для конечного концевого переключателя (концевика)
    quint8 m_bitStartLimitSwitch = 0; // байт для конечного концевика
    quint8 m_bitEndLimitSwitch = 0; // байт для конечного концевика
    //
    qint32 m_maxFrequency = 0;
    qint32 m_minFrequency = 0;
};
#endif //GRADUATOR_G540CONTROLLER_H
