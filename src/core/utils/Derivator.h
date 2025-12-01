#ifndef GRADUATOR_DERIVATOR_H
#define GRADUATOR_DERIVATOR_H

#include <QtGlobal>
#include <deque>

class Derivator final{
public:
    // capacity — максимальное число точек в истории
    explicit Derivator(int capacity = 8);

    // Добавление новой точки
    void push(qreal t, qreal x);

    qreal x() const;

    // Первая производная по двум последним точкам: v = dx/dt
    qreal d() const;

    // Вторая производная: a = d²x/dt² по трем последним точкам
    qreal dd() const;

    // Производная порядка n-точек (finite differences)
    qreal d(int n) const;

private:
    struct Point {
        qreal t;
        qreal x;
    };

    int capacity_;
    std::deque<Point> points_;
};


#endif //GRADUATOR_DERIVATOR_H