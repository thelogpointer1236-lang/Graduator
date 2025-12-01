#include "Mediator.h"

Mediator::Mediator(): mean_(0.0), count_(0) {}

void Mediator::push(qreal x) {
    ++count_;
    mean_ += (x - mean_) / count_;
}

qint64 Mediator::count() const {
    return count_;
}

qreal Mediator::mean() const {
    return mean_;
}

void Mediator::reset() {
    mean_ = 0.0;
    count_ = 0;
}
