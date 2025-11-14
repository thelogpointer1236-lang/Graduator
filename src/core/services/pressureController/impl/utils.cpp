#include "utils.h"
#include "defines.h"
#include "core/services/ServiceLocator.h"
double getPreloadFactor() {
    return ServiceLocator::instance().configManager()->get<double>(CFG_KEY_PRESSURE_PRELOAD_FACTOR);
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