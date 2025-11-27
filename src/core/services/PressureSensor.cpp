#ifndef USE_STUB_IMPLEMENTATIONS

#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include "PressureSensor.h"
#include "core/services/ServiceLocator.h"
#include <QThread>

int getDelayMilliseconds() {
    return ServiceLocator::instance().configManager()->get<int>("pressureSensor.delayMilliseconds", 80);
}
PressureSensor::PressureSensor(QObject *parent)
    : QObject(parent)
      , m_hSerial(INVALID_HANDLE_VALUE)
      , m_unitByteIndex(ServiceLocator::instance().configManager()->get<int>("pressureSensor.unitByteIndex", 0))
      , m_responseLength(ServiceLocator::instance().configManager()->get<int>("pressureSensor.responseLength", 5))
      , m_requestBytes(QByteArray::fromHex(
          ServiceLocator::instance().configManager()->get<QString>("pressureSensor.requestBytes", "01").toLocal8Bit()))
      , m_pressureByteIndices(ServiceLocator::instance().configManager()->get<QVector<int> >("pressureSensor.pressureByteIndices", {4, 3, 2, 1})) {
    m_responseBuffer.resize(64);
}
PressureSensor::~PressureSensor() {
    if (m_isRunning || m_aboutToStop) {
        stop();
    }
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        closeCOM();
    }
}
Pressure PressureSensor::getLastPressure() const {
    return m_lastPressure;
}
bool PressureSensor::openCOM(const QString &comPort, QString &error) {
    if (comPort.isEmpty()) {
        error = QString::fromWCharArray(L"В функцию openCOM передано пустое значение COM-порта.");
        return false;
    }
    m_comPort = comPort;
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        error = QString::fromStdWString(L"COM-порт " + comPort.toStdWString() + L" уже открыт.");
        return false;
    }
    m_hSerial = CreateFileW(
        m_comPort.toStdWString().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (m_hSerial == INVALID_HANDLE_VALUE) {
        const DWORD herr = GetLastError();
        error = QString::fromWCharArray(L"Не удалось открыть COM-порт %1, HRESULT: 0x%2").arg(m_comPort).arg(herr);
        return false;
    }
    // Настройка параметров COM-порта
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(m_hSerial, &dcbSerialParams)) {
        error = QString::fromWCharArray(L"Не удалось получить DCB (LPDCB) состояние COM-порта");
        closeCOM();
        return false;
    }
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(m_hSerial, &dcbSerialParams)) {
        error = QString::fromWCharArray(
            L"Не удалось установить DCB состояние COM-порта (CBR_9600, ONESTOPBIT, NOPARITY)");
        closeCOM();
        return false;
    }
    // Настройка таймаутов
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 500;
    timeouts.ReadTotalTimeoutMultiplier = 8;
    timeouts.WriteTotalTimeoutConstant = 500;
    timeouts.WriteTotalTimeoutMultiplier = 8;
    SetCommTimeouts(m_hSerial, &timeouts);
    // Очистка буферов
    PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"COM-порт %1 успешно открыт").arg(m_comPort));
    return true;
}
void PressureSensor::stop() {
    if (!m_isRunning && !m_aboutToStop) {
        return;
    }
    m_aboutToStop = true;
    while (m_isRunning) {
        QThread::msleep(10);
    }
    m_aboutToStop = false;
    ServiceLocator::instance().logger()->debug(
        QString::fromWCharArray(L"Опрос датчика на порту %1 остановлен").arg(m_comPort));
}
bool PressureSensor::isRunning() const {
    return m_isRunning;
}
void PressureSensor::closeCOM() {
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
        ServiceLocator::instance().logger()->debug(QString::fromWCharArray(L"COM-порт %1 закрыт").arg(m_comPort));
    }
}
void PressureSensor::start() {
    if (m_isRunning) {
        ServiceLocator::instance().logger()->warn(
            QString::fromWCharArray(L"Опрос датчика на порту %1 уже запущен").arg(m_comPort));
        return;
    }
    // Проверка, что индексы данных находятся в пределах ответа
    if (const bool isNotValidIndices = std::any_of(m_pressureByteIndices.begin(), m_pressureByteIndices.end(),
                                                   [&](const int index) { return index >= m_responseLength; });
        m_unitByteIndex >= m_responseLength || isNotValidIndices) {
        ServiceLocator::instance().logger()->error(QString::fromWCharArray(
            L"Индексы давления или единиц измерения вне диапазона ответа на порту %1, требуется проверка настроек").
            arg(m_comPort));
        return;
    }
    m_isRunning = true;
    ServiceLocator::instance().logger()->debug(
        QString::fromWCharArray(L"Опрос датчика на порту %1 запущен").arg(m_comPort));
    int delayMs = getDelayMilliseconds();
    while (true) {
        if (m_aboutToStop || QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
        if (QString err; !pollOnce(err)) {
            ServiceLocator::instance().logger()->error(
                QString::fromWCharArray(L"Ошибка опроса датчика на порту %1: %2").arg(m_comPort, err));
            break;
        }
        QThread::msleep(delayMs);
    }
    m_isRunning = false;
}
bool PressureSensor::pollOnce(QString &error) {
    // 1. Отправка запроса
    DWORD bytesWritten = 0;
    if (!WriteFile(m_hSerial, m_requestBytes.data(), static_cast<DWORD>(m_requestBytes.size()), &bytesWritten,
                   nullptr)) {
        error = QString::fromWCharArray(L"Ошибка отправки запроса на порт %1. Отправлено %2/%3 байт. Код ошибки: 0x%4").
                arg(m_comPort).arg(bytesWritten).arg(m_requestBytes.size()).
                arg(GetLastError(), 8, 16, QLatin1Char('0'));
        return false;
    }
    // 2. Чтение ответа
    DWORD bytesRead = 0;
    if (!ReadFile(m_hSerial, m_responseBuffer.data(), m_responseLength, &bytesRead, nullptr)) {
        error = QString::fromWCharArray(L"Ошибка чтения ответа с порта %1. Код ошибки: 0x%2").arg(m_comPort).arg(
            GetLastError(), 8, 16, QLatin1Char('0'));
        return false;
    }
    // 3. Проверка длины ответа
    if (bytesRead != m_responseLength) {
        ServiceLocator::instance().logger()->debug(
            QString::fromWCharArray(L"Неверный ответ от устройства на порту %1. Получено %2 байт, ожидалось %3").
            arg(m_comPort).arg(bytesRead).arg(m_responseLength));
        return true;
    }
    // 4. Извлечение значения давления
    std::array<std::uint8_t, sizeof(float)> sensorBytes = {
        static_cast<std::uint8_t>(m_responseBuffer[m_pressureByteIndices[0]]),
        static_cast<std::uint8_t>(m_responseBuffer[m_pressureByteIndices[1]]),
        static_cast<std::uint8_t>(m_responseBuffer[m_pressureByteIndices[2]]),
        static_cast<std::uint8_t>(m_responseBuffer[m_pressureByteIndices[3]])
    };

    std::array<std::uint8_t, sizeof(float)> nativeBytes = sensorBytes;
    const bool isAscendingIndices = std::is_sorted(m_pressureByteIndices.cbegin(), m_pressureByteIndices.cend());
    const bool isDescendingIndices = std::is_sorted(m_pressureByteIndices.crbegin(), m_pressureByteIndices.crend());
    const bool isHostLittleEndian = [] {
        const quint16 value = 0x0102;
        return *(reinterpret_cast<const quint8 *>(&value)) == 0x02;
    }();

    if (isAscendingIndices != isDescendingIndices) {
        const bool isDeviceLittleEndian = isDescendingIndices;
        if ((isDeviceLittleEndian && !isHostLittleEndian) || (!isDeviceLittleEndian && isHostLittleEndian)) {
            std::reverse_copy(sensorBytes.cbegin(), sensorBytes.cend(), nativeBytes.begin());
        }
    }

    float pressureValue = 0.0F;
    std::memcpy(&pressureValue, nativeBytes.data(), sizeof(float));
    if (!qIsFinite(pressureValue)) {
        ServiceLocator::instance().logger()->debug(
            QString::fromWCharArray(L"Получено некорректное значение давления с порта %1").arg(m_comPort));
        return true;
    }
    // 6. Извлечение единицы измерения
    const quint8 unitByte = m_responseBuffer[m_unitByteIndex];
    PressureUnit unit;
    switch (unitByte) {
        case 1: unit = PressureUnit::Kgf;
            break;
        case 2: unit = PressureUnit::MPa;
            break;
        case 3: unit = PressureUnit::kPa;
            break;
        case 4: unit = PressureUnit::Pa;
            break;
        case 5: unit = PressureUnit::KgfM2;
            break;
        case 6: unit = PressureUnit::Atm;
            break;
        case 7: unit = PressureUnit::mmHg;
            break;
        case 8: unit = PressureUnit::mmH2O;
            break;
        case 9: unit = PressureUnit::Bar;
            break;
        default:
            ServiceLocator::instance().logger()->debug(
                QString::fromWCharArray(L"Получено недопустимое значение единицы измерения с порта %1: 0x%2").
                arg(m_comPort).arg(unitByte, 2, 16, QLatin1Char('0')));
            return true;
    }
    // 7. Эмиссия сигнала с измеренным давлением
    const qreal timestampSec = ServiceLocator::instance().graduationService()->getElapsedTimeSeconds();
    m_lastPressure = Pressure(pressureValue, unit);
    emit pressureMeasured(timestampSec, m_lastPressure);
    return true;
}

#endif