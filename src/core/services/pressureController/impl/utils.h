#ifndef GRADUATOR_UTILS_H
#define GRADUATOR_UTILS_H

#include "core/types/PressureUnit.h"

#include <QString>
#include <vector>
#include <cmath>


bool tryGetPreloadFactor(PressureUnit unit, double gaugeUpperLimit, double &outFactor, QString *err = nullptr);
double getMaxVelocityFactor();
double getMinVelocityFactor();
double getNominalDurationSec();

bool isNearToPressureNode(
    double p_cur,
    double thr_l,
    double thr_r,
    const std::vector<double>& nodes,
    double& nearing)
{
    nearing = 0.0;

    if (nodes.empty())
        return false;

    if (nodes.size() == 1) {
        double d = std::fabs(p_cur - nodes[0]);
        double thr = p_cur >= nodes[0] ? thr_r : thr_l;
        if (d >= thr) return false;
        nearing = 1.0 - d / thr;
        return true;
    }

    if (p_cur > nodes.back())
        return true;

    if (p_cur < nodes.front())
        return false;

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

    if (min_dist >= thr)
        return false;

    nearing = 1.0 - min_dist / thr;
    return true;
}


#endif //GRADUATOR_UTILS_H
