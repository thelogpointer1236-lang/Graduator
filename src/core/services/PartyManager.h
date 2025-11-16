#ifndef GRADUATOR_PARTYMANAGER_H
#define GRADUATOR_PARTYMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QVector>
#include <QMap>
#include <QPair>

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

    signals:
        void partyNumberChanged(int newPartyNumber);

private:
    void loadOrCreateCurrentParty();
    void createNewParty();
    void updatePartyNumberIfNewDay();

    QByteArray serializeMarks(const QVector<QPair<double,double>> &marks) const;

    QByteArray serializePressure() const;
    QByteArray serializeCameraPressure() const;

    QVector<QPair<double,double>> pressureHistory;
    QMap<int, QVector<QPair<double,double>>> cameraPressureHistory;

    QSqlDatabase db;
    int currentPartyId = -1;
    int partyNumber = 1;
    QDate currentDate;
};

#endif // GRADUATOR_PARTYMANAGER_H
