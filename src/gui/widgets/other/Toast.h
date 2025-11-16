#ifndef GRADUATOR_TOAST_H
#define GRADUATOR_TOAST_H
#include <QWidget>

class Toast final : public QWidget {
public:
    Toast(QWidget *parent, const QString &msg);
};


#endif //GRADUATOR_TOAST_H