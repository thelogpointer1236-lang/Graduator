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
    explicit PartyManager(int standNumber, QObject *parent = nullptr);

    bool savePartyResult(const PartyResult& result, QString& err);

    // Обновление пути + автоматическое определение номера партии
    bool updateCurrentPath(QString& err);

    QString currentPath() const;

    int partyNumber() const;

    // Запросили явно: увеличить номер партии
    void incrementPartyNumber();

    void setCurrentPressureUnit(int index);
    void setCurrentPrecision(int index);
    void setCurrentDisplacement(int index);
    void setCurrentPrinter(int index);

    int currentPressureUnitIndex() const;

    int currentPrecisionIndex() const;

    int currentDisplacementIndex() const;

    int currentPrinterIndex() const;

    const QStringList& getAvailablePressureUnits() const;
    const QStringList& getAvailablePrecisions() const;
    const QList<Displacement>& getAvailableDisplacements() const;
    const QList<Printer>& getAvailablePrinters() const;

private:
    void initializeAvailableOptions();

    // Сканирует текущий путь и определяет следующий номер партии
    void updatePartyNumberFromFileSystem();

    QString m_currentPath;   // итоговый путь
    QString m_currentDate;   // текущая дата (строкой для контроля смены дня)
    int m_standNumber;       // номер стенда
    int m_partyNumber;       // номер партии

    QStringList m_availablePressureUnits;
    QStringList m_availablePrecisions;
    QList<Displacement> m_availableDisplacements;
    QList<Printer> m_availablePrinters;
};



#endif //GRADUATOR_PARTYMANAGER_H