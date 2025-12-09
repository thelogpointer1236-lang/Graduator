#ifndef GRADUATOR_UTILS_H
#define GRADUATOR_UTILS_H

#include "core/types/PressureUnit.h"

#include <QString>

bool tryGetPreloadFactor(PressureUnit unit, double gaugeUpperLimit, double &outFactor, QString *err = nullptr);
double getMaxVelocityFactor();
double getMinVelocityFactor();
double getNominalDurationSec();

#endif //GRADUATOR_UTILS_H
