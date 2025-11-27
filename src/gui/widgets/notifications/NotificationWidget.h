#ifndef GRADUATOR_NOTIFICATIONWIDGET_H
#define GRADUATOR_NOTIFICATIONWIDGET_H

#include <QWidget>
#include "core/services/Logger.h"

class QLabel;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
class QTimer;

class NotificationWidget final : public QWidget {
    Q_OBJECT
public:
    explicit NotificationWidget(const QString &text, LogLevel level, QWidget *parent = nullptr);
    ~NotificationWidget() override = default;

    void showAnimated();
    void dismiss();

signals:
    void finished(NotificationWidget *self);

private:
    void setupUi(const QString &text, LogLevel level);
    void startFadeOut();

    QLabel *label_{};
    QGraphicsOpacityEffect *opacityEffect_{};
    QPropertyAnimation *fadeAnimation_{};
    QTimer *lifetimeTimer_{};
    bool closing_ = false;
};

#endif // GRADUATOR_NOTIFICATIONWIDGET_H
