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
    double* nearing);

double lerp(double from, double to, double t);

#endif //GRADUATOR_UTILS_H
