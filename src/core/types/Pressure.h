#ifndef GRADUATOR_PRESSURE_H
#define GRADUATOR_PRESSURE_H
#include <QObject>
#include "PressureUnit.h"
#include <stdexcept>
class Pressure {
public:
    Pressure() : m_kpa(0.0), m_punit(PressureUnit::kpa) {
    }
    Pressure(double value, PressureUnit unit) { setValue(value, unit); }
    // фабрики
    static Pressure fromKPa(double value) { return Pressure(value, PressureUnit::kpa); }
    static Pressure fromMPa(double value) { return Pressure(value, PressureUnit::mpa); }
    static Pressure fromBar(double value) { return Pressure(value, PressureUnit::bar); }
    static Pressure fromKgf(double value) { return Pressure(value, PressureUnit::kgf); }
    static Pressure fromAtm(double value) { return Pressure(value, PressureUnit::atm); }
    // конверторы
    double toPa() const { return m_kpa * 1000.0; }
    double toKPa() const { return m_kpa; }
    double toMPa() const { return m_kpa / 1000.0; }
    double toBar() const { return m_kpa / 100.0; }
    double toKgf() const { return m_kpa / 98.0665; }
    double toKgfM2() const { return m_kpa * 1000.0 / 9.80665; } // 1 kgf/m² = 9.80665 Pa
    double toAtm() const { return m_kpa / 101.325; } // 1 atm = 101.325 kPa
    double toMmHg() const { return m_kpa * 1000.0 / 133.322; } // 1 mmHg = 133.322 Pa
    double toMmH2O() const { return m_kpa * 1000.0 / 9.80665; } // 1 mmH2O = 9.80665 Pa
    // геттеры
    double getValue() const { return m_kpa; }
    double getValue(PressureUnit unit) const {
        switch (unit) {
            case PressureUnit::kpa: return toKPa();
            case PressureUnit::mpa: return toMPa();
            case PressureUnit::bar: return toBar();
            case PressureUnit::kgf: return toKgf();
            case PressureUnit::atm: return toAtm();
            default: throw std::invalid_argument("Unknown unit");
        }
    }
    // сеттер
    void setValue(double value, PressureUnit unit) {
        switch (unit) {
            case PressureUnit::kpa: m_kpa = value;
                break;
            case PressureUnit::mpa: m_kpa = value * 1000.0;
                break;
            case PressureUnit::bar: m_kpa = value * 100.0;
                break;
            case PressureUnit::kgf: m_kpa = value * 98.0665;
                break;
            case PressureUnit::atm: m_kpa = value * 101.325;
                break;
            default: throw std::invalid_argument("Unknown unit");
        }
        m_punit = unit;
    }
    PressureUnit unit() const { return m_punit; }
private:
    double m_kpa;
    PressureUnit m_punit;
};
Q_DECLARE_METATYPE(Pressure)
#endif // GRADUATOR_PRESSURE_H