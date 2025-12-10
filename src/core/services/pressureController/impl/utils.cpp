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
        case PressureUnit::Pa: return "Pa";
        case PressureUnit::kPa: return "kPa";
        case PressureUnit::MPa: return "MPa";
        case PressureUnit::Bar: return "Bar";
        case PressureUnit::Kgf: return "kgf/cm";
        case PressureUnit::KgfM2: return "kgf/m";
        case PressureUnit::Atm: return "atm";
        case PressureUnit::mmHg: return "mmHg";
        case PressureUnit::mmH2O: return "mmH2O";
        case PressureUnit::Unknown: break;
    }
    return {};
}
}

bool tryGetPreloadFactor(PressureUnit unit, double gaugeUpperLimit, double &outFactor, QString *err) {
    auto *configManager = ServiceLocator::instance().configManager();
    if (!configManager) {
        if (err) *err = QObject::tr("Конфигурационный файл не найден.");
        return false;
    }

    const QJsonValue preloadConfig = configManager->getValue(CFG_KEY_PRESSURE_PRELOAD_FACTOR);
    if (!preloadConfig.isObject()) {
        if (err) *err = QObject::tr("Некорректно задан коэффициент преднагрузки в настройках.");
        return false;
    }

    const QString unitName = pressureUnitKey(unit);
    if (unitName.isEmpty()) {
        if (err) *err = QObject::tr("Не выбрана корректная единица измерения для коэффициента преднагрузки.");
        return false;
    }

    const QString gaugeKey = QString::number(static_cast<int>(qRound(gaugeUpperLimit)));
    const auto preloadByGauge = preloadConfig.toObject();
    if (!preloadByGauge.contains(gaugeKey) || !preloadByGauge.value(gaugeKey).isObject()) {
        if (err) *err = QObject::tr("Не задан коэффициент преднагрузки для предела %1.").arg(gaugeKey);
        return false;
    }

    const auto unitObject = preloadByGauge.value(gaugeKey).toObject();
    const auto unitValue = unitObject.value(unitName);
    if (!unitValue.isDouble()) {
        if (err) *err = QObject::tr("Не задан коэффициент преднагрузки для предела %1 в единице %2.")
                                      .arg(gaugeKey, unitName);
        return false;
    }

    outFactor = unitValue.toDouble();
    return true;
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
