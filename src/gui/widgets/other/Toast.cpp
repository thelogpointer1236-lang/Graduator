#include "Toast.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QTimer>

Toast::Toast(QWidget *parent, const QString &msg): QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *label = new QLabel(msg, this);
    label->setStyleSheet("background: #c62828; color: white; padding: 8px; "
        "border-radius: 4px;");

    auto layout = new QHBoxLayout(this);
    layout->addWidget(label);

    // расположение
    adjustSize();
    QPoint pos = parent->rect().bottomRight() - QPoint(width() + 10, height() + 10);
    move(parent->mapToGlobal(pos));

    // анимация появления
    auto *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    show();

    // исчезновение
    QTimer::singleShot(3000, this, [this]{
        auto *anim = new QPropertyAnimation(this, "windowOpacity");
        anim->setDuration(300);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        connect(anim, &QPropertyAnimation::finished, this, &Toast::deleteLater);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}
