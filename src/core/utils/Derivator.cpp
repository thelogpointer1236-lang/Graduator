#include "Derivator.h"

Derivator::Derivator(int capacity): capacity_(capacity) {}

void Derivator::push(qreal t, qreal x) {
    points_.push_back({t, x});
    if (points_.size() > capacity_) {
        points_.pop_front();
    }
}

qreal Derivator::x() const {
     return points_.front().x;
}

qreal Derivator::d() const {
    if (points_.size() < 2) return 0.0;

    const auto& p1 = points_[points_.size() - 2];
    const auto& p2 = points_[points_.size() - 1];

    qreal dt = p2.t - p1.t;
    if (dt == 0.0) return 0.0;
    return (p2.x - p1.x) / dt;
}

qreal Derivator::dd() const {
    if (points_.size() < 3) return 0.0;

    const auto& p0 = points_[points_.size() - 3];
    const auto& p1 = points_[points_.size() - 2];
    const auto& p2 = points_[points_.size() - 1];

    qreal dt1 = p1.t - p0.t;
    qreal dt2 = p2.t - p1.t;

    if (dt1 == 0.0 || dt2 == 0.0) return 0.0;

    qreal v1 = (p1.x - p0.x) / dt1;
    qreal v2 = (p2.x - p1.x) / dt2;

    qreal dt = (dt1 + dt2) / 2.0;
    return (v2 - v1) / dt;
}


qreal Derivator::d(int n) const {
    if (n < 2) return 0.0;

    if (points_.size() < n) return d(points_.size());

    // Берём последние n точек
    int start = points_.size() - n;

    // Если точек мало, делаем одностороннюю разность
    if (n == 2) {
        const auto& p1 = points_[start];
        const auto& p2 = points_[start + 1];
        qreal dt = p2.t - p1.t;
        if (dt == 0.0) return 0.0;
        return (p2.x - p1.x) / dt;
    }

    // Если n >= 3, делаем центральные разности
    // Формула: f'(t_k) ≈ (x[k+1] - x[k-1]) / (t[k+1] - t[k-1])
    // Берём центральную точку
    int mid = start + n / 2;

    if (mid - 1 < 0 || mid + 1 >= points_.size()) return 0.0;

    const auto& pL = points_[mid - 1];
    const auto& pR = points_[mid + 1];

    qreal dt = pR.t - pL.t;
    if (dt == 0.0) return 0.0;

    return (pR.x - pL.x) / dt;
}
