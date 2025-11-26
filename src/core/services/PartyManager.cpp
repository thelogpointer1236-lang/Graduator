#include "PartyManager.h"
#include "defines.h"
#include "ServiceLocator.h"

void PartyManager::setCurrentPressureUnit(int index) {
    if (index < 0 || index >= m_availablePressureUnits.size()) return;
    m_currentPressureUnitIndex = index;
}

void PartyManager::setCurrentPrecision(int index) {
    if (index < 0 || index >= m_availablePrecisions.size()) return;
    m_currentPrecisionIndex = index;
}

void PartyManager::setCurrentDisplacement(int index) {
    if (index < 0 || index >= m_availableDisplacements.size()) return;
    m_currentDisplacementIndex = index;
}

void PartyManager::setCurrentPrinter(int index) {
    if (index < 0 || index >= m_availablePrinters.size()) return;
    m_currentPrinterIndex = index;
}

const QStringList& PartyManager::getAvailablePressureUnits() const {
    return m_availablePressureUnits;
}

const QStringList& PartyManager::getAvailablePrecisions() const {
    return m_availablePrecisions;
}

const QList<Displacement>& PartyManager::getAvailableDisplacements() const {
    return m_availableDisplacements;
}

const QList<Printer>& PartyManager::getAvailablePrinters() const {
    return m_availablePrinters;
}

void PartyManager::initializeAvailableOptions() {
    m_availablePressureUnits = QStringList{
        tr("Pa"),          // Pa
        tr("kPa"),         // kPa
        tr("MPa"),         // MPa
        tr("Bar"),         // Bar
        tr("kgf/cm"),         // Kgf
        tr("kgf/m"),      // KgfM2
        tr("atm"),         // Atm
        tr("mmHg"),        // mmHg
        tr("mmH2O"),       // mmH2O
    };

    {
        auto x = ServiceLocator::instance().configManager()->get<QVector<double>>(CFG_KEY_PRECISION_CLASSES, QVector<double>{});
        for (const double &v : x)
            m_availablePrecisions.append(QString::number(v));
    }
    {
        auto x = ServiceLocator::instance().configManager()->getValue(CFG_KEY_PRINTERS).toArray();
        if (!x.isEmpty()) {
            for (const auto &item : x) {
                m_availablePrinters.append(
                    Printer(
                        item.toObject().value("name").toString(),
                        item.toObject().value("folder").toString()
                    )
                );
            }
        }
    }
    {
        auto x = ServiceLocator::instance().configManager()->getValue(CFG_KEY_DISPLACEMENTS).toArray();
        if (!x.isEmpty()) {
            for (const auto &item : x) {
                m_availableDisplacements.append(
                    Displacement(
                        item.toObject().value("name").toString(),
                        item.toObject().value("num").toInt()
                    )
                );
            }
        }
    }
}
