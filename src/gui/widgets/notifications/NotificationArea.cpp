#include "NotificationArea.h"

#include "NotificationWidget.h"

#include <QEvent>
#include <QtGlobal>
#include <QVBoxLayout>

namespace {
    constexpr int kMaxNotifications = 2;
    constexpr int kHorizontalMargin = 12;
    constexpr int kVerticalMargin = 12;
    constexpr int kMaxWidth = 360;
}

NotificationArea::NotificationArea(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    layout_->setContentsMargins(kHorizontalMargin, kVerticalMargin, kHorizontalMargin, kVerticalMargin);
    layout_->setSpacing(8);

    reposition();

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
    reposition();
}

bool NotificationArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        reposition();
    }
    return QWidget::eventFilter(watched, event);
}

void NotificationArea::handleFinished(NotificationWidget *widget)
{
    notifications_.removeOne(widget);
    layout_->removeWidget(widget);
    widget->deleteLater();
    reposition();
}

void NotificationArea::reposition()
{
    if (!parentWidget()) {
        return;
    }

    const auto hint = layout_->sizeHint();
    const int availableWidth = qMax(0, parentWidget()->width() - (kHorizontalMargin * 2));
    const int width = qMin(kMaxWidth, availableWidth);
    const int availableHeight = qMax(0, parentWidget()->height() - (kVerticalMargin * 2));
    const int height = qMin(availableHeight, hint.height());
    const int x = parentWidget()->width() - width - kHorizontalMargin;
    const int y = parentWidget()->height() - height - kVerticalMargin;

    setFixedWidth(width);
    setFixedHeight(height);
    setGeometry(x, y, width, height);
}