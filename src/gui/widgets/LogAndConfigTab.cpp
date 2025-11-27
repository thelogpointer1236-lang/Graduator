#include "LogAndConfigTab.h"

#include "core/services/ConfigManager.h"
#include "core/services/ServiceLocator.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QVector>
#include <QStringList>

namespace {
    class JsonHighlighter final : public QSyntaxHighlighter {
    public:
        explicit JsonHighlighter(QTextDocument *parent = nullptr)
            : QSyntaxHighlighter(parent) {
            setupRules();
        }

    protected:
        void highlightBlock(const QString &text) override {
            for (const auto &rule : rules_) {
                QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
                while (matchIterator.hasNext()) {
                    const auto match = matchIterator.next();
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }
        }

    private:
        struct HighlightingRule {
            QRegularExpression pattern;
            QTextCharFormat format;
        };

        void setupRules() {
            QTextCharFormat keyFormat;
            keyFormat.setForeground(QColor(0, 102, 204));
            rules_.append({QRegularExpression("\"(\\\\.|[^\\"])*\"(?=\\s*:)"), keyFormat});

            QTextCharFormat stringFormat;
            stringFormat.setForeground(QColor(0, 153, 0));
            rules_.append({QRegularExpression("\"(\\\\.|[^\\"])*\""), stringFormat});

            QTextCharFormat numberFormat;
            numberFormat.setForeground(QColor(204, 51, 0));
            rules_.append({QRegularExpression("-?\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?\b"), numberFormat});

            QTextCharFormat boolNullFormat;
            boolNullFormat.setForeground(QColor(153, 0, 153));
            boolNullFormat.setFontWeight(QFont::Bold);
            rules_.append({QRegularExpression("\\b(true|false|null)\\b"), boolNullFormat});
        }

        QVector<HighlightingRule> rules_;
    };
}

LogAndConfigTab::LogAndConfigTab(QWidget *parent)
    : QWidget(parent) {
    if (auto *logger = ServiceLocator::instance().logger()) {
        logFilePath_ = logger->logFilePath();
        connect(logger, &Logger::newLogEntry, this, &LogAndConfigTab::appendLog);
    }

    if (auto *cfg = ServiceLocator::instance().configManager()) {
        configFilePath_ = cfg->configPath();
    }

    setupUi();

    reloadLogs();
    reloadConfig();
}

void LogAndConfigTab::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    auto *splitter = new QSplitter(Qt::Vertical, this);

    splitter->addWidget(createLogSection());
    splitter->addWidget(createConfigSection());
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

QWidget *LogAndConfigTab::createLogSection() {
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel(tr("Логи"), this);
    auto *reloadButton = new QPushButton(tr("Обновить"), this);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(reloadButton);

    logView_ = new QPlainTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    logView_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    layout->addLayout(headerLayout);
    layout->addWidget(logView_);

    connect(reloadButton, &QPushButton::clicked, this, &LogAndConfigTab::reloadLogs);

    return container;
}

QWidget *LogAndConfigTab::createConfigSection() {
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel(tr("config.json"), this);
    auto *reloadButton = new QPushButton(tr("Сбросить"), this);
    saveButton_ = new QPushButton(tr("Сохранить"), this);
    restartButton_ = new QPushButton(tr("Перезапустить"), this);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(reloadButton);
    headerLayout->addWidget(saveButton_);
    headerLayout->addWidget(restartButton_);

    configEditor_ = new QPlainTextEdit(this);
    configEditor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    configEditor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    highlighter_ = new JsonHighlighter(configEditor_->document());

    layout->addLayout(headerLayout);
    layout->addWidget(configEditor_);

    connect(reloadButton, &QPushButton::clicked, this, &LogAndConfigTab::reloadConfig);
    connect(saveButton_, &QPushButton::clicked, this, &LogAndConfigTab::saveConfig);
    connect(restartButton_, &QPushButton::clicked, this, &LogAndConfigTab::restartApplication);

    return container;
}

void LogAndConfigTab::reloadLogs() {
    QFile file(logFilePath_);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logView_->setPlainText(tr("Не удалось открыть %1").arg(logFilePath_));
        return;
    }

    logView_->setPlainText(QString::fromUtf8(file.readAll()));
    logView_->verticalScrollBar()->setValue(logView_->verticalScrollBar()->maximum());
}

void LogAndConfigTab::reloadConfig() {
    QFile file(configFilePath_);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        configEditor_->setPlainText(tr("Не удалось открыть %1").arg(configFilePath_));
        saveButton_->setEnabled(false);
        restartButton_->setEnabled(false);
        return;
    }

    saveButton_->setEnabled(true);
    restartButton_->setEnabled(true);
    configEditor_->setPlainText(QString::fromUtf8(file.readAll()));
}

void LogAndConfigTab::appendLog(LogLevel, const QString &text) {
    if (!logView_) {
        return;
    }

    logView_->appendPlainText(text.trimmed());
    logView_->verticalScrollBar()->setValue(logView_->verticalScrollBar()->maximum());
}

void LogAndConfigTab::saveConfig() {
    writeConfigToDisk();
}

bool LogAndConfigTab::writeConfigToDisk() {
    QJsonParseError parseError{};
    const auto content = configEditor_->toPlainText();
    const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, tr("Ошибка JSON"),
                             tr("Не удалось разобрать config.json: %1")
                                 .arg(parseError.errorString()));
        return false;
    }

    QFile file(configFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Ошибка записи"),
                             tr("Не удалось сохранить %1").arg(configFilePath_));
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (auto *cfg = ServiceLocator::instance().configManager()) {
        cfg->load();
    }

    return true;
}

void LogAndConfigTab::restartApplication() {
    if (!writeConfigToDisk()) {
        return;
    }

    const QString program = QCoreApplication::applicationFilePath();
    const QStringList arguments = QCoreApplication::arguments();

    if (!QProcess::startDetached(program, arguments)) {
        QMessageBox::warning(this, tr("Перезапуск"), tr("Не удалось перезапустить приложение."));
        return;
    }

    QCoreApplication::quit();
}
