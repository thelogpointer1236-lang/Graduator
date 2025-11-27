#ifndef GRADUATOR_NOTIFICATIONAREA_H
#define GRADUATOR_NOTIFICATIONAREA_H

#include <QWidget>
#include <QVector>
#include "core/services/Logger.h"

class QVBoxLayout;
class NotificationWidget;
class QEvent;

class NotificationArea final : public QWidget {
    Q_OBJECT

public:
    explicit NotificationArea(QWidget *parent = nullptr);
    ~NotificationArea() override = default;

    void showMessage(LogLevel level, const QString &text);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handleFinished(NotificationWidget *widget);

private:
    void resizeToParent();

    QVBoxLayout *layout_{};
    QVector<NotificationWidget *> notifications_;
};

#endif // GRADUATOR_NOTIFICATIONAREA_H
