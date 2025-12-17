#include "utils.h"

#include "defines.h"
#include "core/services/ServiceLocator.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QtGlobal>

namespace {
QString pressureUnitKey(PressureUnit unit) {
    switch (unit) {
        case PressureUnit::kpa: return "kPa";
        case PressureUnit::mpa: return "MPa";
        case PressureUnit::bar: return "Bar";
        case PressureUnit::kgf: return "kgf/cm";
        case PressureUnit::atm: return "atm";
        case PressureUnit::unknown: break;
    }
    return {};
}
}

double getMaxVelocityFactor() {
    return ServiceLocator::instance().configManager()->get<double>(CFG_KEY_PRESSURE_MAX_VELOCITY_FACTOR);
}

double getMinVelocityFactor() {
    return ServiceLocator::instance().configManager()->get<double>(CFG_KEY_PRESSURE_MIN_VELOCITY_FACTOR);
}

double getNominalDurationSec() {
    return ServiceLocator::instance().configManager()->get<double>(CFG_KEY_PRESSURE_NOMINAL_DURATION_SEC);
}

bool isNearToPressureNode(double p_cur, double thr_l, double thr_r, const std::vector<double> &nodes, double *nearing_) {
    double nearing = 0.0;

    if (nodes.empty())
        return false;

    if (nodes.size() == 1) {
        double d = std::fabs(p_cur - nodes[0]);
        double thr = p_cur >= nodes[0] ? thr_r : thr_l;
        if (d >= thr) return false;
        nearing = 1.0 - d / thr;
        return true;
    }

    if (p_cur > nodes.back()) {
        if (nearing_) *nearing_ = 1.0;
        return true;
    }

    if (p_cur < nodes.front()) {
        if (nearing_) *nearing_ = 0.0;
        return false;
    }

    // Найти ближайший узел (при желании заменим на binary_search)
    double closest = nodes[0];
    double min_dist = std::fabs(p_cur - closest);

    for (double n : nodes) {
        double d = std::fabs(p_cur - n);
        if (d < min_dist) {
            min_dist = d;
            closest = n;
        }
    }

    // Определяем какой порог использовать — слева или справа
    double thr = (p_cur >= closest ? thr_r : thr_l);

    if (min_dist >= thr) {
        if (nearing_) *nearing_ = 0.0;
        return false;
    }

    nearing = 1.0 - min_dist / thr;

    if (nearing_) *nearing_ = nearing;

    return true;
}

double lerp(double from, double to, double t) {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return from + (to - from) * t;
}
