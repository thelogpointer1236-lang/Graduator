#ifndef GRADUATOR_WINRING0WRAPPER_H
#define GRADUATOR_WINRING0WRAPPER_H
#include <QLibrary>
#include <QString>
class LPTDriver {
public:
    LPTDriver();
    ~LPTDriver();
    bool isLoaded() const;
    bool initialize() const;
    quint8 readPort(quint16 port) const;
    void writePort(quint16 port, quint8 value) const;
    QString lastError() const;
private:
    using InitializeOls_t = int();
    using ReadIoPortByte_t = unsigned char(unsigned short);
    using WriteIoPortByte_t = void(unsigned short, unsigned char);
    QLibrary m_lib;
    bool m_loaded;
    mutable QString m_lastError;
    InitializeOls_t *m_initializeOls = nullptr;
    ReadIoPortByte_t *m_readIoPortByte = nullptr;
    WriteIoPortByte_t *m_writeIoPortByte = nullptr;
};
#endif //GRADUATOR_WINRING0WRAPPER_H