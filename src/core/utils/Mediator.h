#ifndef GRADUATOR_MEDIATOR_H
#define GRADUATOR_MEDIATOR_H
#include <QtGlobal>

class Mediator {
public:
    Mediator();
    void push(qreal x);
    qint64 count() const;
    qreal mean() const;
    void reset();
private:
    qreal mean_;
    qint64 count_;
};

#endif //GRADUATOR_MEDIATOR_H