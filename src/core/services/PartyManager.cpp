#include "PartyManager.h"
#include "defines.h"
#include "ServiceLocator.h"
#include <QtMath>

PartyManager::PartyManager(int standNumber, QObject *parent)
    : QObject(parent)
    , m_standNumber(standNumber)
    , m_partyNumber(0)
{
    initializeAvailableOptions();
}

bool PartyManager::savePartyResult(const PartyResult &result, QString &err) {
    if (!updateCurrentPath(err)) {
        return false;
    }

    QDir folder = QString("%1/p%2-%3/")
            .arg(m_currentPath)
            .arg(m_partyNumber)
            .arg(currentDisplacementNumber());

    if (!folder.exists() && !folder.mkpath(".")) {
        err = QString("Cannot create path: %1").arg(folder.path());
        return false;
    }

    for (int camIdx = 0; camIdx < result.forward.size(); ++camIdx) {
        if (result.forward[camIdx].size() != result.gaugeModel.pressureValues().size() ||
            result.backward[camIdx].size() != result.gaugeModel.pressureValues().size()) {
            continue;
        }
        QString fileName = folder.filePath(QString("scale%1.tbl").arg(camIdx + 1));
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            err = QString("Cannot open file for writing: %1").arg(fileName);
            return false;
        }
        QTextStream out(&file);
        out << "       320       240       230\n";
        out << result.gaugeModel.name() << "\n";
        for (int i = 0; i < result.gaugeModel.pressureValues().size(); ++i) {
            const qreal p = result.forward[camIdx][i].pressure;
            const qreal fw = qRadiansToDegrees(result.forward[camIdx][i].angle);
            const qreal bw = qRadiansToDegrees(result.backward[camIdx][i].angle);


            out << QString(" %1 %2 %3 %4 %5\n")
                    .arg(QString::asprintf("% .11E", p))
                    .arg(QString::asprintf("% .11E", fw))
                    .arg(QString::asprintf("% .11E", bw))
                    .arg(QString::asprintf("% .11E", 0))
                    .arg(QString::asprintf("% .11E", 0));
        }
        double durationSec = result.durationSeconds;
        int minutes = static_cast<int>(durationSec) / 60;
        int seconds = static_cast<int>(durationSec) % 60;
        out << tr("Graduation time: %1:%2 sec.\n")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
        file.close();
    }
    if (auto *graduationService = ServiceLocator::instance().graduationService()) {
        graduationService->markResultSaved();
    }
    return true;
}

bool PartyManager::updateCurrentPath(QString &err) {
    QString dateStr = QDate::currentDate().toString("dd.MM.yyyy");

    // Если день сменился – сбрасываем номер партии, будем определять заново
    if (m_currentDate != dateStr) {
        m_currentDate = dateStr;
        m_partyNumber = 0; // признак, что нужно пересчитать по ФС
    }

    QString fullPath = QString("%1/stend%2/%3")
            .arg(m_availablePrinters[currentPrinterIndex()].folder())
            .arg(m_standNumber)
            .arg(dateStr);

    QDir dir(fullPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            err = QString("Cannot create path: %1").arg(fullPath);
            return false;
        }
    }

    m_currentPath = fullPath;

    // Если номер партии ещё не определён – считаем его по файловой структуре
    if (m_partyNumber == 0) {
        updatePartyNumberFromFileSystem();
    }

    return true;
}

QString PartyManager::currentPath() const {
    return m_currentPath;
}

int PartyManager::partyNumber() const {
    return m_partyNumber;
}

void PartyManager::incrementPartyNumber() {
    ++m_partyNumber;
    emit partyNumberChanged(m_partyNumber);
}

void PartyManager::updatePartyNumberFromFileSystem() {
    if (m_currentPath.isEmpty())
        return;

    QDir dir(m_currentPath);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    int maxParty = 0;
    const QStringList entries = dir.entryList();
    QRegularExpression re("^p(\\d+)-(\\d+)"); // p<номер>-<смещение>

    for (const QString& name : entries) {
        QRegularExpressionMatch match = re.match(name);
        if (match.hasMatch()) {
            bool okParty = false;
            bool okDisplacement = false;
            const int party = match.captured(1).toInt(&okParty);
            const int displacement = match.captured(2).toInt(&okDisplacement);
            if (okParty && okDisplacement && displacement == currentDisplacementNumber() && party > maxParty) {
                maxParty = party;
            }
        }
    }

    int newPartyNumber = (maxParty == 0) ? 1 : (maxParty + 1);

    if (m_partyNumber != newPartyNumber) {
        m_partyNumber = newPartyNumber;
        emit partyNumberChanged(m_partyNumber);
    }
}

int PartyManager::currentPressureUnitIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRESSURE_UNIT, 0);
}

int PartyManager::currentPrecisionIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRECISION_CLASS, 0);
}

int PartyManager::currentDisplacementIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_DISPLACEMENT, 0);
}

int PartyManager::currentDisplacementNumber() const {
    const int index = currentDisplacementIndex();
    if (index < 0 || index >= m_availableDisplacements.size()) {
        return index;
    }
    return m_availableDisplacements.at(index).num();
}

int PartyManager::currentPrinterIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRINTER, 0);
}

void PartyManager::setCurrentPressureUnit(int index) {
    if (index < 0 || index >= m_availablePressureUnits.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRESSURE_UNIT, index);
}

void PartyManager::setCurrentPrecision(int index) {
    if (index < 0 || index >= m_availablePrecisions.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRECISION_CLASS, index);
}

void PartyManager::setCurrentDisplacement(int index) {
    if (index < 0 || index >= m_availableDisplacements.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_DISPLACEMENT, index);
}

void PartyManager::setCurrentPrinter(int index) {
    if (index < 0 || index >= m_availablePrinters.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRINTER, index);
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
