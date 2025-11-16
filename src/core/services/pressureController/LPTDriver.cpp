#include "LPTDriver.h"
LPTDriver::LPTDriver() : m_lib("WinRing0x64"), m_loaded(false) {
    if (!m_lib.load()) {
        m_lastError = QString::fromWCharArray(L"Не удалось загрузить WinRing0x64.dll");
        return;
    }

    m_initializeOls = reinterpret_cast<InitializeOls_t *>(m_lib.resolve("InitializeOls"));
    m_readIoPortByte = reinterpret_cast<ReadIoPortByte_t *>(m_lib.resolve("ReadIoPortByte"));
    m_writeIoPortByte = reinterpret_cast<WriteIoPortByte_t *>(m_lib.resolve("WriteIoPortByte"));

    if (!m_initializeOls || !m_readIoPortByte || !m_writeIoPortByte) {
        m_lastError = QString::fromWCharArray(
            L"Не удалось разрешить функции InitializeOls, ReadIoPortByte, WriteIoPortByte из WinRing0x64.dll");
        m_lib.unload();
        return;
    }

    m_loaded = true;
}
LPTDriver::~LPTDriver() {
    if (m_lib.isLoaded())
        m_lib.unload();
}
bool LPTDriver::isLoaded() const { return m_loaded; }
bool LPTDriver::initialize() const {
    if (!m_loaded || !m_initializeOls) {
        if (m_lastError.isEmpty())
            m_lastError = QString::fromWCharArray(
                L"WinRing0x64.dll не загружена или отсутствуют необходимые функции");
        return false;
    }

    switch (m_initializeOls()) {
        case 1:
            m_lastError = "";
            return true;
        case 0:
            m_lastError = QString::fromWCharArray(L"WinRing0x64: Не удалось загрузить драйвер WinRing0");
            return false;
        case -1:
            m_lastError = QString::fromWCharArray(L"WinRing0x64: Нет прав администратора для доступа к порту");
            return false;
        case -2:
            m_lastError = QString::fromWCharArray(L"WinRing0x64: Несовместимая версия ОС");
            return false;
        default:
            m_lastError = QString::fromWCharArray(L"WinRing0x64: Неизвестная ошибка при инициализации драйвера");
            return false;
    }
}
quint8 LPTDriver::readPort(quint16 port) const { return m_readIoPortByte(port); }
void LPTDriver::writePort(quint16 port, quint8 value) const { m_writeIoPortByte(port, value); }
QString LPTDriver::lastError() const { return m_lastError; }