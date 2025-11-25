#ifndef GRADUATOR_PARTYMANAGER_H
#define GRADUATOR_PARTYMANAGER_H

#include <QDate>
#include <QDir>
#include <QFile>
#include <qregularexpression.h>
#include <QTextStream>

#include "core/types/Displacement.h"
#include "core/types/PartyResult.h"
#include "core/types/PressureUnit.h"
#include "core/types/Printer.h"

class PartyManager final : public QObject {
    Q_OBJECT
signals:
    void partyNumberChanged(int newPartyNumber);

public:
    explicit PartyManager(int standNumber, QObject *parent = nullptr)
        : QObject(parent)
        , m_currentPath()
        , m_currentDate()
        , m_standNumber(standNumber)
        , m_partyNumber(0)
        , m_currentDisplacementIndex(0)
        , m_currentPrinterIndex(0)
        , m_currentPressureUnitIndex(0)
        , m_currentPrecisionIndex(0) {
        initializeAvailableOptions();
    }

    bool savePartyResult(const PartyResult& result, QString& err) {
        if (!updateCurrentPath(err)) {
            return false;
        }

        QDir folder = QString("%1/p%2-%3/")
                .arg(m_currentPath)
                .arg(m_partyNumber)
                .arg(m_currentDisplacementIndex);

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
                out << QString(" %1 %2 %3 %4 %5\n")
                        .arg(QString::asprintf("% .11E", result.forward[camIdx][i].pressure))
                        .arg(QString::asprintf("% .11E", result.forward[camIdx][i].angle))
                        .arg(QString::asprintf("% .11E", result.backward[camIdx][i].angle))
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
        return true;
    }

    // Обновление пути + автоматическое определение номера партии
    bool updateCurrentPath(QString& err) {
        QString dateStr = QDate::currentDate().toString("dd.MM.yyyy");

        // Если день сменился – сбрасываем номер партии, будем определять заново
        if (m_currentDate != dateStr) {
            m_currentDate = dateStr;
            m_partyNumber = 0; // признак, что нужно пересчитать по ФС
        }

        QString fullPath = QString("%1/stand%2/%3")
                .arg(m_availablePrinters[m_currentPrinterIndex].folder())
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

    QString currentPath() const {
        return m_currentPath;
    }

    int partyNumber() const {
        return m_partyNumber;
    }

    // Запросили явно: увеличить номер партии
    void incrementPartyNumber() {
        ++m_partyNumber;
        emit partyNumberChanged(m_partyNumber);
    }

    void setCurrentPressureUnit(int index);
    void setCurrentPrecision(int index);
    void setCurrentDisplacement(int index);
    void setCurrentPrinter(int index);

    const QStringList& getAvailablePressureUnits() const;
    const QStringList& getAvailablePrecisions() const;
    const QList<Displacement>& getAvailableDisplacements() const;
    const QList<Printer>& getAvailablePrinters() const;

private:
    void initializeAvailableOptions();

    // Сканирует текущий путь и определяет следующий номер партии
    void updatePartyNumberFromFileSystem() {
        if (m_currentPath.isEmpty())
            return;

        QDir dir(m_currentPath);
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

        int maxParty = 0;
        const QStringList entries = dir.entryList();
        QRegularExpression re("^p(\\d+)-"); // p<номер>-<смещение>

        for (const QString& name : entries) {
            QRegularExpressionMatch match = re.match(name);
            if (match.hasMatch()) {
                bool ok = false;
                int party = match.captured(1).toInt(&ok);
                if (ok && party > maxParty) {
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

    QString m_currentPath;   // итоговый путь
    QString m_currentDate;   // текущая дата (строкой для контроля смены дня)
    int m_standNumber;       // номер стенда
    int m_partyNumber;       // номер партии

    int m_currentDisplacementIndex;
    int m_currentPrinterIndex;
    int m_currentPressureUnitIndex;
    int m_currentPrecisionIndex;
    QStringList m_availablePressureUnits;
    QStringList m_availablePrecisions;
    QList<Displacement> m_availableDisplacements;
    QList<Printer> m_availablePrinters;
};



#endif //GRADUATOR_PARTYMANAGER_H