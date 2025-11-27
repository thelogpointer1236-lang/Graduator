#include "NotificationArea.h"

#include "NotificationWidget.h"

#include <QEvent>
#include <QVBoxLayout>

namespace {
    constexpr int kMaxNotifications = 3;
    constexpr int kHorizontalMargin = 12;
    constexpr int kVerticalMargin = 12;
}

NotificationArea::NotificationArea(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    layout_->setContentsMargins(kHorizontalMargin, kVerticalMargin, kHorizontalMargin, kVerticalMargin);
    layout_->setSpacing(8);

    resizeToParent();

    if (parent) {
        parent->installEventFilter(this);
    }
}

void NotificationArea::showMessage(LogLevel level, const QString &text)
{
    auto *toast = new NotificationWidget(text, level, this);
    connect(toast, &NotificationWidget::finished, this, &NotificationArea::handleFinished);

    if (notifications_.size() >= kMaxNotifications) {
        auto *oldest = notifications_.takeFirst();
        layout_->removeWidget(oldest);
        oldest->dismiss();
    }

    notifications_.append(toast);
    layout_->addWidget(toast);
    toast->showAnimated();
}

bool NotificationArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        resizeToParent();
    }
    return QWidget::eventFilter(watched, event);
}

void NotificationArea::handleFinished(NotificationWidget *widget)
{
    notifications_.removeOne(widget);
    layout_->removeWidget(widget);
    widget->deleteLater();
}

void NotificationArea::resizeToParent()
{
    if (auto *parent = parentWidget()) {
        setGeometry(parent->rect());
    }
}
