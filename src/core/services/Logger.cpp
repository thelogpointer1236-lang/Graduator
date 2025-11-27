#include "Logger.h"
#include <QDir>
#include <QTextCodec>

Logger::Logger(QObject *parent) : QObject(parent) {
}

bool Logger::init(const QString &logFilePath) {
    QDir().mkpath(QFileInfo(logFilePath).absolutePath());
    m_file.setFileName(logFilePath);
    if (!m_file.open(QIODevice::Append | QIODevice::Text))
        return false;
    m_stream.setDevice(&m_file);
    m_stream.setCodec("UTF-8");
    return true;
}

void Logger::write(LogLevel level, const QString &message) {
    QMutexLocker lock(&m_mutex);
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString line = QString("[%1] [%2] %3\n")
            .arg(time, levelToString(level), message);
    m_stream << line;
    m_stream.flush();
    emit newLogEntry(level, line);
}

void Logger::write(LogLevel level, const wchar_t *message) {
    write(level, QString::fromWCharArray(message));
}
void Logger::debug(const QString &msg) { write(LogLevel::Debug, msg); }
void Logger::debug(const wchar_t *msg) { write(LogLevel::Debug, msg); }
void Logger::info(const QString &msg) { write(LogLevel::Info, msg); }
void Logger::info(const wchar_t *msg) { write(LogLevel::Info, msg); }
void Logger::warn(const QString &msg) { write(LogLevel::Warning, msg); }
void Logger::warn(const wchar_t *msg) { write(LogLevel::Warning, msg); }
void Logger::error(const QString &msg) { write(LogLevel::Error, msg); }
void Logger::error(const wchar_t *msg) { write(LogLevel::Error, msg); }
void Logger::critical(const QString &msg) { write(LogLevel::Critical, msg); }
void Logger::critical(const wchar_t *msg) { write(LogLevel::Critical, msg); }

QString Logger::logFilePath() const {
    return m_file.fileName();
}
QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}
