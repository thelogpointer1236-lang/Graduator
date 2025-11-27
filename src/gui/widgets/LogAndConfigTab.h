#ifndef GRADUATOR_LOGANDCONFIGTAB_H
#define GRADUATOR_LOGANDCONFIGTAB_H

#include <QWidget>
#include "core/services/Logger.h"

class QPlainTextEdit;
class QPushButton;
class QSyntaxHighlighter;

class LogAndConfigTab final : public QWidget {
    Q_OBJECT
public:
    explicit LogAndConfigTab(QWidget *parent = nullptr);

private slots:
    void reloadLogs();
    void reloadConfig();
    void appendLog(LogLevel level, const QString &text);
    void saveConfig();
    void restartApplication();

private:
    void setupUi();
    QWidget *createLogSection();
    QWidget *createConfigSection();
    bool writeConfigToDisk();
    void reloadFilteredLogs();

private:
    QString logFilePath_ = "logs.txt";
    QString configFilePath_ = "config.json";

    std::vector<std::pair<LogLevel, QString>> logBuffer_;
    LogLevel currentFilterLevel_ = LogLevel::Debug;

    QPlainTextEdit *logView_ = nullptr;
    QPlainTextEdit *configEditor_ = nullptr;
    QPushButton *saveButton_ = nullptr;
    QPushButton *restartButton_ = nullptr;
    QSyntaxHighlighter *highlighter_ = nullptr;
};

#endif // GRADUATOR_LOGANDCONFIGTAB_H
