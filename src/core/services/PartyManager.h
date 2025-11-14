#ifndef GRADUATOR_PARTYMANAGER_H
#define GRADUATOR_PARTYMANAGER_H
#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QVector>
#include <QPair>
class PartyManager final : public QObject {
    Q_OBJECT
public:
    explicit PartyManager(QObject *parent = nullptr);
    bool initDatabase(const QString &path);
    int currentPartyNumber() const;
    void startNewMeasurement();
    void addGauge(const QString &name, const QVector<QPair<double, double> > &marks);
    void finalizeCurrentParty();
    signals:
    
    void partyNumberChanged(int newPartyNumber);
private:
    void loadOrCreateCurrentParty();
    void createNewParty();
    void updatePartyNumberIfNewDay();
    QByteArray serializeMarks(const QVector<QPair<double, double> > &marks) const;
    QVector<QPair<double, double> > deserializeMarks(const QByteArray &blob) const;
    QSqlDatabase db;
    int currentPartyId = -1;
    int partyNumber = 1;
    QDate currentDate;
};
#endif //GRADUATOR_PARTYMANAGER_H