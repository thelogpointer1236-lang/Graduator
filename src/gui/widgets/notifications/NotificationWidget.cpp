#include "NotificationWidget.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

namespace {
    constexpr int kFadeInDurationMs = 150;
    constexpr int kFadeOutDurationMs = 200;
    constexpr int kDisplayDurationMs = 4500;

    QString backgroundForLevel(LogLevel level) {
        switch (level) {
            case LogLevel::Warning:
                return "rgba(255, 193, 7, 200)";      // Amber
            case LogLevel::Error:
            case LogLevel::Critical:
                return "rgba(220, 53, 69, 210)";     // Red
            default:
                return "rgba(23, 162, 184, 200)";    // Teal for informational messages
        }
    }

    QString borderForLevel(LogLevel level) {
        switch (level) {
            case LogLevel::Warning:
                return "rgba(255, 193, 7, 240)";
            case LogLevel::Error:
            case LogLevel::Critical:
                return "rgba(185, 40, 55, 240)";
            default:
                return "rgba(18, 130, 147, 240)";
        }
    }

    QString textColorForLevel(LogLevel level) {
        switch (level) {
            case LogLevel::Warning:
                return "#2a1c02";
            case LogLevel::Error:
            case LogLevel::Critical:
                return "#ffffff";
            default:
                return "#06272d";
        }
    }
}

NotificationWidget::NotificationWidget(const QString &text, LogLevel level, QWidget *parent)
    : QWidget(parent)
{
    setupUi(text, level);
}

void NotificationWidget::setupUi(const QString &text, LogLevel level)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);

    label_ = new QLabel(text, this);
    label_->setWordWrap(true);
    label_->setStyleSheet(QString("color: %1; font-weight: 600;")
                              .arg(textColorForLevel(level)));

    layout->addWidget(label_);

    const QString style = QString(
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: 10px;"
    ).arg(backgroundForLevel(level), borderForLevel(level));

    setStyleSheet(style);

    opacityEffect_ = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect_);
    opacityEffect_->setOpacity(0.0);

    fadeAnimation_ = new QPropertyAnimation(opacityEffect_, "opacity", this);

    lifetimeTimer_ = new QTimer(this);
    lifetimeTimer_->setInterval(kDisplayDurationMs);
    lifetimeTimer_->setSingleShot(true);
    connect(lifetimeTimer_, &QTimer::timeout, this, &NotificationWidget::startFadeOut);

    connect(fadeAnimation_, &QPropertyAnimation::finished, this, [this]() {
        if (closing_) {
            emit finished(this);
        }
    });
}

void NotificationWidget::showAnimated()
{
    fadeAnimation_->stop();
    closing_ = false;

    fadeAnimation_->setDuration(kFadeInDurationMs);
    fadeAnimation_->setStartValue(0.0);
    fadeAnimation_->setEndValue(1.0);
    fadeAnimation_->start();

    lifetimeTimer_->start();
}

void NotificationWidget::dismiss()
{
    startFadeOut();
}

void NotificationWidget::startFadeOut()
{
    if (closing_) {
        return;
    }

    closing_ = true;
    lifetimeTimer_->stop();

    fadeAnimation_->stop();
    fadeAnimation_->setDuration(kFadeOutDurationMs);
    fadeAnimation_->setStartValue(opacityEffect_->opacity());
    fadeAnimation_->setEndValue(0.0);
    fadeAnimation_->start();
}
