#include <Windows.h>
#include "G540Driver.h"
#include "core/services/ServiceLocator.h"
#include "defines.h"
#include <QThread>
G540Driver::G540Driver(QObject *parent) : QObject(parent) {
    m_LPTDriver = std::make_unique<LPTDriver>();
    if (!m_LPTDriver->isLoaded()) {
        m_lastError = m_LPTDriver->lastError();
        return;
    }
    if (!m_LPTDriver->initialize()) {
        m_lastError = m_LPTDriver->lastError();
        return;
    }
    setDirection(m_direction);
    // Параметры:
    auto &cfg = *ServiceLocator::instance().configManager();
    try {
        m_portAddress = cfg.get<quint16>(CFG_KEY_G540_PORT_ADDRESS);
        m_byteCloseBothFlaps = cfg.get<int>(CFG_KEY_G540_BYTE_CLOSE_BOTH_FLAPS);
        m_byteOpenInputFlap = cfg.get<int>(CFG_KEY_G540_BYTE_OPEN_INPUT_FLAP);
        m_byteOpenOutputFlap = cfg.get<int>(CFG_KEY_G540_BYTE_OPEN_OUTPUT_FLAP);
        m_bitStartLimitSwitch = cfg.get<int>(CFG_KEY_G540_BIT_START_LIMIT_SWITCH);
        m_bitEndLimitSwitch = cfg.get<int>(CFG_KEY_G540_BIT_END_LIMIT_SWITCH);
        m_maxFrequency = cfg.get<int>(CFG_KEY_G540_MAX_FREQUENCY);
        m_minFrequency = cfg.get<int>(CFG_KEY_G540_MIN_FREQUENCY);
    } catch (...) {
        m_lastError = "Invalid configuration parameter in G540Driver";
        return;
    }
}
G540Driver::~G540Driver() {
    stop();
}
void G540Driver::setDirection(G540Direction direction) {
    m_direction = direction;
    constexpr int axis = 0; // Ось X
    constexpr int shift = 2 * axis; // 0, 2, 4, 6 для X,Y,Z,A
    if (m_direction == G540Direction::Forward) {
        m_byte1 = static_cast<quint8>(0 << shift);
        m_byte2 = static_cast<quint8>(1 << shift);
    } else {
        m_byte1 = static_cast<quint8>(2 << shift);
        m_byte2 = static_cast<quint8>(3 << shift);
    }
}
G540Direction G540Driver::direction() const {
    return m_direction;
}
void G540Driver::setDangerousPressure(qreal dangerousPressure) {
    m_dangerousPressure = dangerousPressure;
}
void G540Driver::updatePressure(qreal pressure) {
    m_currentPressure = pressure;
    m_lastTimeMsDangP = m_elapsedTimerDangP.elapsed();
}
void G540Driver::setFrequency(const int frequency) {
    m_lastTimeMs = m_elapsedTimer.elapsed();
    if (frequency <= m_minFrequency / 4 || frequency <= 0) {
        m_frequency = 0U;
        m_halfDelayMcs = 0U;
        return;
    }
    if (frequency < m_minFrequency)
        m_frequency = static_cast<quint32>(m_minFrequency);
    else if (frequency > m_maxFrequency)
        m_frequency = static_cast<quint32>(m_maxFrequency);
    else
        m_frequency = static_cast<quint32>(frequency);
    m_halfDelayMcs = 500000U / m_frequency;
}
quint32 G540Driver::frequency() const {
    return m_frequency;
}
quint32 G540Driver::impulsesCount() const {
    return m_impulsesCount;
}
quint32 G540Driver::maxFrequency() const {
    return m_maxFrequency;
}
quint32 G540Driver::minFrequency() const {
    return m_minFrequency;
}
bool G540Driver::isAnyLimitTriggered() const {
    return state() & (byteStartLimitSwitch() | byteEndLimitSwitch());
}

#include <iostream>
bool G540Driver::isStartLimitTriggered() const {
    const bool sw = state() & byteStartLimitSwitch();
    m_startLimitTriggered = sw;
    return sw;
}
bool G540Driver::isEndLimitTriggered() const {
    const bool sw = state() & byteEndLimitSwitch();
    m_endLimitTriggered = sw;
    return sw;
}
void G540Driver::setFlapsState(G540FlapsState flapState) {
    m_flapsState = flapState;
    quint8 currentPortValue;
    // В зависимости от типа клапана и действия обновляем состояние
    switch (flapState) {
        case G540FlapsState::CloseBoth:
            currentPortValue = m_byteCloseBothFlaps;
            break;
        case G540FlapsState::OpenInput:
            currentPortValue = m_byteOpenInputFlap;
            break;
        case G540FlapsState::OpenOutput:
            currentPortValue = m_byteOpenOutputFlap;
            break;
        default:
            return;
    }
    // Записываем обновлённое состояние на порт
    writeOffsetPortByte(2, currentPortValue);
}
G540FlapsState G540Driver::flapsState() const {
    return m_flapsState;
}
qreal G540Driver::timeSinceStartSec() const {
    if (!m_isRunning) return 0.0;
    return static_cast<qreal>(m_elapsedTimer.elapsed()) / 1000.0;
}
bool G540Driver::isReadyToStart(QString &err) const {
    err = m_lastError;
    return m_lastError.isEmpty();
}
void G540Driver::start() {
    setFrequency(0);
    m_impulsesCount = 0;
    m_elapsedTimerDangP.start();
    m_elapsedTimer.start();
    m_lastTimeMsDangP = 0;
    m_lastTimeMs = 0;
    m_isRunning = true;
    while (true) {
        if (m_aboutToStop || QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
         // Задаваться частота и давление должны минимум раз в 250 мс и 500 мс соответственно
         if (m_elapsedTimer.elapsed() - m_lastTimeMs > 250/* || m_elapsedTimerDangP.elapsed() - m_lastTimeMsDangP > 500*/) {
             delayMicroseconds(15000);
             continue;
         }
         // Проверка концевиков
         if ((isStartLimitTriggered() && m_direction == G540Direction::Backward) ||
             (isEndLimitTriggered() && m_direction == G540Direction::Forward)) {
             delayMicroseconds(15000);
             continue;
         }
        if (m_frequency == 0U || m_halfDelayMcs == 0U) {
            delayMicroseconds(15000);
            continue;
        }
        writePortByte(m_byte1);
        delayMicroseconds(m_halfDelayMcs);
        writePortByte(m_byte2);
        delayMicroseconds(m_halfDelayMcs);
        m_impulsesCount++;
    }
    m_isRunning = false;
}
void G540Driver::stop() {
    m_aboutToStop = true;
    while (m_isRunning) {
        QThread::msleep(10);
    }
    m_aboutToStop = false;
}
bool G540Driver::isRunning() const {
    return m_isRunning;
}
void G540Driver::delayMicroseconds(const long long target) {
    if (target == 0) return;
    LARGE_INTEGER start, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    const long double ticksPerMicrosecond = static_cast<long double>(freq.QuadPart) / 1000000.0L;
    const long long requiredTicks = static_cast<long long>(target * ticksPerMicrosecond);
    while (true) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (now.QuadPart - start.QuadPart >= requiredTicks) break;
    }
}
quint8 G540Driver::state() const {
    return readOffsetPortByte(1) ^ (1 << 7);
}
void G540Driver::writePortByte(quint8 byte) const {
    m_LPTDriver->writePort(m_portAddress, byte);
}
quint8 G540Driver::readPortByte() const {
    return m_LPTDriver->readPort(m_portAddress);
}
void G540Driver::writeOffsetPortByte(quint16 offset, quint8 byte) const {
    m_LPTDriver->writePort(m_portAddress + offset, byte);
}
quint8 G540Driver::readOffsetPortByte(quint16 offset) const {
    return m_LPTDriver->readPort(m_portAddress + offset);
}

quint8 G540Driver::byteStartLimitSwitch() const {
    return (quint8)(1<<m_bitStartLimitSwitch);
}
quint8 G540Driver::byteEndLimitSwitch() const {
    return (quint8)(1<<m_bitEndLimitSwitch);
}
