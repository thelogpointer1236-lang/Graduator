#ifndef GRADUATOR_PARTYMANAGER_H
#define GRADUATOR_PARTYMANAGER_H

#include <QDate>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <functional>

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

    // Основной метод сохранения результата
    bool savePartyResult(const PartyResult& result, QString& err);

    // Обновление каталога и номера партии
    bool updateCurrentPath(QString& err);

    QString currentPath() const;
    int partyNumber() const;

    void incrementPartyNumber();

    // Настройки
    void setCurrentPressureUnit(int index);
    void setCurrentPrecision(int index);
    void setCurrentDisplacement(int index);
    void setCurrentPrinter(int index);

    int currentPressureUnitIndex() const;
    int currentPrecisionIndex() const;
    double currentPrecisionValue() const;

    int currentDisplacementIndex() const;
    int currentDisplacementNumber() const;

    int currentPrinterIndex() const;

    const QStringList& getAvailablePressureUnits() const;
    const QStringList& getAvailablePrecisions() const;
    const QList<Displacement>& getAvailableDisplacements() const;
    const QList<Printer>& getAvailablePrinters() const;

    // Новый механизм подтверждения сохранения результата.
    // UI подписывается сюда, возвращает true/false.
    std::function<bool(const QString& question)> confirmCallback;

private:
    void initializeAvailableOptions();
    void updatePartyNumberFromFileSystem();

private:
    QString m_currentPath;
    QString m_currentDate;

    int m_standNumber;
    int m_partyNumber;

    QStringList m_availablePressureUnits;
    QStringList m_availablePrecisions;
    QList<Displacement> m_availableDisplacements;
    QList<Printer> m_availablePrinters;
};

#endif // GRADUATOR_PARTYMANAGER_H
