#ifndef GRADUATOR_PRESSURE_H
#define GRADUATOR_PRESSURE_H
#include "PressureUnit.h"
#include <stdexcept>
class Pressure {
public:
    Pressure() : m_kpa(0.0), m_punit(PressureUnit::kPa) {
    }
    Pressure(double value, PressureUnit unit) { setValue(value, unit); }
    // фабрики
    static Pressure fromPa(double value) { return Pressure(value, PressureUnit::Pa); }
    static Pressure fromKPa(double value) { return Pressure(value, PressureUnit::kPa); }
    static Pressure fromMPa(double value) { return Pressure(value, PressureUnit::MPa); }
    static Pressure fromBar(double value) { return Pressure(value, PressureUnit::Bar); }
    static Pressure fromKgf(double value) { return Pressure(value, PressureUnit::Kgf); }
    static Pressure fromKgfM2(double value) { return Pressure(value, PressureUnit::KgfM2); }
    static Pressure fromAtm(double value) { return Pressure(value, PressureUnit::Atm); }
    static Pressure fromMmHg(double value) { return Pressure(value, PressureUnit::mmHg); }
    static Pressure fromMmH2O(double value) { return Pressure(value, PressureUnit::mmH2O); }
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
            case PressureUnit::Pa: return toPa();
            case PressureUnit::kPa: return toKPa();
            case PressureUnit::MPa: return toMPa();
            case PressureUnit::Bar: return toBar();
            case PressureUnit::Kgf: return toKgf();
            case PressureUnit::KgfM2: return toKgfM2();
            case PressureUnit::Atm: return toAtm();
            case PressureUnit::mmHg: return toMmHg();
            case PressureUnit::mmH2O: return toMmH2O();
            default: throw std::invalid_argument("Unknown unit");
        }
    }
    // сеттер
    void setValue(double value, PressureUnit unit) {
        switch (unit) {
            case PressureUnit::Pa: m_kpa = value / 1000.0;
                break;
            case PressureUnit::kPa: m_kpa = value;
                break;
            case PressureUnit::MPa: m_kpa = value * 1000.0;
                break;
            case PressureUnit::Bar: m_kpa = value * 100.0;
                break;
            case PressureUnit::Kgf: m_kpa = value * 98.0665;
                break;
            case PressureUnit::KgfM2: m_kpa = value * 9.80665 / 1000.0;
                break;
            case PressureUnit::Atm: m_kpa = value * 101.325;
                break;
            case PressureUnit::mmHg: m_kpa = value * 133.322 / 1000.0;
                break;
            case PressureUnit::mmH2O: m_kpa = value * 9.80665 / 1000.0;
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