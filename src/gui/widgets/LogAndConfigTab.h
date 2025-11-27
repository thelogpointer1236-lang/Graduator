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

private:
    QPlainTextEdit *logView_ = nullptr;
    QPlainTextEdit *configEditor_ = nullptr;
    QPushButton *saveButton_ = nullptr;
    QPushButton *restartButton_ = nullptr;
    QSyntaxHighlighter *highlighter_ = nullptr;

    QString logFilePath_ = "logs.txt";
    QString configFilePath_ = "config.json";
};

#endif // GRADUATOR_LOGANDCONFIGTAB_H
