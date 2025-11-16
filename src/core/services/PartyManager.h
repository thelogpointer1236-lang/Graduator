#ifndef GRADUATOR_PARTYMANAGER_H
#define GRADUATOR_PARTYMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QPair>
#include "core/types/PartyResult.h"

struct PartyHistoryRecord {
    int id = -1;
    int partyNumber = 0;
    QDateTime startTime;
    QDateTime endTime;
    bool hasResult = false;

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

class PartyManager final : public QObject {
    Q_OBJECT
public:
    explicit PartyManager(QObject *parent = nullptr);

    bool initDatabase(const QString &path);
    int currentPartyNumber() const;

    void startNewMeasurement();
    void finalizeCurrentParty();

    void addGauge(const QString &name, const QVector<QPair<double,double>> &marks);

    void addPressureSample(double timestamp, double pressure);
    void addCameraPressureSample(int camera, double timestamp, double pressure);

    QVector<PartyHistoryRecord> partyHistory() const;
    PartyResult loadPartyResult(int partyId) const;
    bool saveCurrentPartyResult(const PartyResult &result);

signals:
    void partyNumberChanged(int newPartyNumber);
    void historyChanged();

private:
    void loadOrCreateCurrentParty();
    void createNewParty();
    void updatePartyNumberIfNewDay();
    void ensureResultColumns();

    QByteArray serializeMarks(const QVector<QPair<double,double>> &marks) const;

    QByteArray serializePressure() const;
    QByteArray serializeCameraPressure() const;
    QByteArray serializeResult(const std::vector<std::vector<double>> &result) const;
    std::vector<std::vector<double>> deserializeResult(const QByteArray &blob) const;

    QVector<QPair<double,double>> pressureHistory;
    QMap<int, QVector<QPair<double,double>>> cameraPressureHistory;

    QSqlDatabase db;
    int currentPartyId = -1;
    int partyNumber = 1;
    QDate currentDate;
};

#endif // GRADUATOR_PARTYMANAGER_H
