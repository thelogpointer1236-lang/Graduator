#ifndef GRADUATOR_LOGGER_H
#define GRADUATOR_LOGGER_H
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
enum class LogLevel { Debug, Info, Warning, Error, Critical };
class Logger final : public QObject {
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    bool init(const QString &logFilePath);
    void write(LogLevel level, const QString &message);
    void write(LogLevel level, const wchar_t *message);
    void debug(const QString &msg);
    void debug(const wchar_t *msg);
    void info(const QString &msg);
    void info(const wchar_t *msg);
    void warn(const QString &msg);
    void warn(const wchar_t *msg);
    void error(const QString &msg);
    void error(const wchar_t *msg);
    void critical(const QString &msg);
    void critical(const wchar_t *msg);
signals:
    void newLogEntry(LogLevel level, const QString &text);

private:
    QString levelToString(LogLevel level) const;
    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;
};
#endif //GRADUATOR_LOGGER_H
/*  Пример использвоания:
    log.init("app.log");
    log.debug("Debug message");
    log.info(L"Info message");
    log.warn("Warning message");
    log.error(L"Error message");
 */