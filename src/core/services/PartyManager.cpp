#include "PartyManager.h"
#include "defines.h"
#include "ServiceLocator.h"

#include <QtMath>
#include <limits>
#include <algorithm>
#include <QMessageBox>
#include <QDebug>
#include <QSet>

PartyManager::PartyManager(int standNumber, QObject *parent)
    : QObject(parent)
    , m_standNumber(standNumber)
    , m_partyNumber(0)
{
    initializeAvailableOptions();

    QString err;
    if (!updateCurrentPath(err)) {
        qWarning() << "Failed to initialize party path:" << err;
    }
}

// ============================================================================
//                               SAVE RESULT
// ============================================================================

bool PartyManager::savePartyResult(const PartyResult &result, QString &err)
{
    // 0. Глобальная валидация перед любыми действиями
    const PartyValidationResult validation = result.validate();

    QSet<int> camerasWithErrors;
    for (const auto &issue : validation.issues) {
        if (issue.severity == PartyValidationIssue::Severity::Error && issue.cameraIndex >= 0) {
            camerasWithErrors.insert(issue.cameraIndex);
        }
    }

    // 1. Подтверждение сохранения результата
    if (confirmCallback) {
        if (!confirmCallback(tr("Вы уверены, что хотите сохранить результат?"))) {
            err = tr("Сохранение отменено пользователем.");
            return false;
        }
    }

    // 2. Обновляем путь + определяем номер партии
    if (!updateCurrentPath(err)) {
        return false;
    }

    QDir folder(QString("%1/p%2-%3/")
                .arg(m_currentPath)
                .arg(m_partyNumber)
                .arg(currentDisplacementNumber()));

    if (!folder.exists() && !folder.mkpath(".")) {
        err = QString("Cannot create path: %1").arg(folder.path());
        return false;
    }

    const int camCount = std::min(result.forward.size(), result.backward.size());
    QStringList unsavedGauges;
    bool anyCameraSaved = false;

    for (int camIdx = 0; camIdx < camCount; ++camIdx) {

        // Можно удалить skipCam — теперь ошибки блокируют всё сохранение заранее.
        // Но если хочешь оставить — он просто не сыграет роли, так как ошибок уже не должно быть.

        if (camerasWithErrors.contains(camIdx) ||
            result.forward[camIdx].size() != result.gaugeModel.pressureValues().size() ||
            result.backward[camIdx].size() != result.gaugeModel.pressureValues().size()) {
            unsavedGauges << QString::number(camIdx + 1);
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

        const auto &pressures = result.gaugeModel.pressureValues();

        for (int i = 0; i < pressures.size(); ++i) {
            const qreal p  = result.forward[camIdx][i].pressure;
            const qreal fw = qRadiansToDegrees(result.forward[camIdx][i].angle);
            const qreal bw = qRadiansToDegrees(result.backward[camIdx][i].angle);

            out << QString(" %1 %2 %3 %4 %5\n")
                   .arg(QString::asprintf("% .11E", p))
                   .arg(QString::asprintf("% .11E", fw))
                   .arg(QString::asprintf("% .11E", bw))
                   .arg(QString::asprintf("% .11E", 0.0))
                   .arg(QString::asprintf("% .11E", 0.0));
        }

        double durationSec = result.durationSeconds;
        int minutes = static_cast<int>(durationSec) / 60;
        int seconds = static_cast<int>(durationSec) % 60;

        out << tr("Graduation time: %1:%2 sec.\n")
               .arg(minutes, 2, 10, QChar('0'))
               .arg(seconds, 2, 10, QChar('0'));

        file.close();
        anyCameraSaved = true;
    }

    if (!anyCameraSaved) {
        err = tr("Cannot save result: all gauges contain errors or missing measurements.");
        return false;
    }

    if (!unsavedGauges.isEmpty()) {
        QMessageBox::warning(
            nullptr,
            tr("Save warning"),
            tr("All gauges were saved successfully except %1.").arg(unsavedGauges.join(", ")));
    } else {
        QMessageBox::information(nullptr, tr("Save result"), tr("All gauges were saved successfully."));
    }

    incrementPartyNumber();
    return true;
}


// ============================================================================
//                               PATH UPDATE
// ============================================================================

bool PartyManager::updateCurrentPath(QString &err)
{
    QString dateStr = QDate::currentDate().toString("dd.MM.yyyy");

    // День сменился – сбрасываем номер партии
    if (m_currentDate != dateStr) {
        m_currentDate = dateStr;
        m_partyNumber = 0;
    }

    QString fullPath;

    const int printerIdx = currentPrinterIndex();
    if (printerIdx < 0 || printerIdx >= m_availablePrinters.size()) {
        fullPath = QString("stend%1/%2")
                       .arg(m_standNumber)
                       .arg(dateStr);
    } else {
        fullPath = QString("%1/stend%2/%3")
                       .arg(m_availablePrinters[printerIdx].folder())
                       .arg(m_standNumber)
                       .arg(dateStr);
    }

    QDir dir(fullPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        err = QString("Cannot create path: %1").arg(fullPath);
        return false;
    }

    m_currentPath = fullPath;

    if (m_partyNumber == 0)
        updatePartyNumberFromFileSystem();

    return true;
}

QString PartyManager::currentPath() const { return m_currentPath; }

int PartyManager::partyNumber() const { return m_partyNumber; }

void PartyManager::incrementPartyNumber()
{
    ++m_partyNumber;
    emit partyNumberChanged(m_partyNumber);
}

// ============================================================================
//                        AUTO-DETECT PARTY NUMBER
// ============================================================================

void PartyManager::updatePartyNumberFromFileSystem()
{
    if (m_currentPath.isEmpty())
        return;

    QDir dir(m_currentPath);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    int maxParty = 0;
    const QStringList entries = dir.entryList();
    QRegularExpression re("^p(\\d+)-(\\d+)");

    for (const QString &name : entries) {
        QRegularExpressionMatch match = re.match(name);
        if (!match.hasMatch())
            continue;

        bool ok1 = false, ok2 = false;
        const int party = match.captured(1).toInt(&ok1);
        const int disp  = match.captured(2).toInt(&ok2);

        if (ok1 && ok2 &&
            disp == currentDisplacementNumber() &&
            party > maxParty)
        {
            maxParty = party;
        }
    }

    int newNum = (maxParty == 0) ? 1 : maxParty + 1;

    if (newNum != m_partyNumber) {
        m_partyNumber = newNum;
        emit partyNumberChanged(m_partyNumber);
    }
}

// ============================================================================
//                                SETTINGS
// ============================================================================

int PartyManager::currentPressureUnitIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRESSURE_UNIT, 0);
}

int PartyManager::currentPrecisionIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRECISION_CLASS, 0);
}

double PartyManager::currentPrecisionValue() const
{
    const int idx = currentPrecisionIndex();
    if (idx < 0 || idx >= m_availablePrecisions.size())
        return std::numeric_limits<double>::quiet_NaN();

    bool ok = false;
    double v = m_availablePrecisions[idx].toDouble(&ok);
    return ok ? v : std::numeric_limits<double>::quiet_NaN();
}

int PartyManager::currentDisplacementIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_DISPLACEMENT, 0);
}

int PartyManager::currentDisplacementNumber() const
{
    const int index = currentDisplacementIndex();
    if (index < 0 || index >= m_availableDisplacements.size())
        return index;

    return m_availableDisplacements[index].num();
}

int PartyManager::currentPrinterIndex() const {
    return ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_PRINTER, 0);
}

void PartyManager::setCurrentPressureUnit(int index)
{
    if (index < 0 || index >= m_availablePressureUnits.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRESSURE_UNIT, index);
}

void PartyManager::setCurrentPrecision(int index)
{
    if (index < 0 || index >= m_availablePrecisions.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRECISION_CLASS, index);
}

void PartyManager::setCurrentDisplacement(int index)
{
    if (index < 0 || index >= m_availableDisplacements.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_DISPLACEMENT, index);
}

void PartyManager::setCurrentPrinter(int index)
{
    if (index < 0 || index >= m_availablePrinters.size()) return;
    ServiceLocator::instance().configManager()->setValue(CFG_KEY_CURRENT_PRINTER, index);
}

const QStringList& PartyManager::getAvailablePressureUnits() const { return m_availablePressureUnits; }
const QStringList& PartyManager::getAvailablePrecisions() const { return m_availablePrecisions; }
const QList<Displacement>& PartyManager::getAvailableDisplacements() const { return m_availableDisplacements; }
const QList<Printer>& PartyManager::getAvailablePrinters() const { return m_availablePrinters; }

// ============================================================================
//                             INIT OPTIONS
// ============================================================================

void PartyManager::initializeAvailableOptions()
{
    m_availablePressureUnits = QStringList{
        tr("Pa"),
        tr("kPa"),
        tr("MPa"),
        tr("Bar"),
        tr("kgf/cm"),
        tr("kgf/m"),
        tr("atm"),
        tr("mmHg"),
        tr("mmH2O"),
    };

    {
        auto x = ServiceLocator::instance().configManager()->get<QVector<double>>(
            CFG_KEY_PRECISION_CLASSES,
            QVector<double>{}
        );

        for (double v : x)
            m_availablePrecisions.append(QString::number(v));
    }

    {
        auto x = ServiceLocator::instance().configManager()->getValue(CFG_KEY_PRINTERS).toArray();
        for (const auto &item : x) {
            m_availablePrinters.append(
                Printer(
                    item.toObject().value("name").toString(),
                    item.toObject().value("folder").toString()
                )
            );
        }
    }

    {
        auto x = ServiceLocator::instance().configManager()->getValue(CFG_KEY_DISPLACEMENTS).toArray();
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
