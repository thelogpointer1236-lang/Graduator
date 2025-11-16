#include "PartyManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>

PartyManager::PartyManager(QObject *parent)
    : QObject(parent)
{
}

bool PartyManager::initDatabase(const QString &path)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open())
        return false;

    QSqlQuery q;

    q.exec("CREATE TABLE IF NOT EXISTS parties ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "party_number INTEGER,"
           "start_time TEXT,"
           "end_time TEXT,"
           "pressure_history BLOB,"
           "camera_pressure_history BLOB)");

    q.exec("CREATE TABLE IF NOT EXISTS gauges ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "party_id INTEGER,"
           "name TEXT,"
           "marks BLOB,"
           "FOREIGN KEY(party_id) REFERENCES parties(id))");

    updatePartyNumberIfNewDay();
    loadOrCreateCurrentParty();

    return true;
}

void PartyManager::updatePartyNumberIfNewDay()
{
    currentDate = QDate::currentDate();
    QSqlQuery q;
    q.prepare("SELECT MAX(party_number) FROM parties WHERE date(start_time)=?");
    q.addBindValue(currentDate.toString(Qt::ISODate));

    if (q.exec() && q.next() && !q.value(0).isNull())
        partyNumber = q.value(0).toInt() + 1;
    else
        partyNumber = 1;
}

void PartyManager::loadOrCreateCurrentParty()
{
    QSqlQuery q;
    q.prepare("INSERT INTO parties (party_number, start_time) VALUES (?, datetime('now'))");
    q.addBindValue(partyNumber);
    q.exec();
    currentPartyId = q.lastInsertId().toInt();
}

void PartyManager::startNewMeasurement()
{
    partyNumber++;
    createNewParty();
}

void PartyManager::createNewParty()
{
    pressureHistory.clear();
    cameraPressureHistory.clear();

    QSqlQuery q;
    q.prepare("INSERT INTO parties (party_number, start_time) VALUES (?, datetime('now'))");
    q.addBindValue(partyNumber);
    q.exec();

    currentPartyId = q.lastInsertId().toInt();
    emit partyNumberChanged(partyNumber);
}

int PartyManager::currentPartyNumber() const
{
    return partyNumber;
}

void PartyManager::addGauge(const QString &name, const QVector<QPair<double,double>> &marks)
{
    if (currentPartyId < 0)
        return;

    QSqlQuery q;
    q.prepare("INSERT INTO gauges (party_id, name, marks) VALUES (?, ?, ?)");
    q.addBindValue(currentPartyId);
    q.addBindValue(name);
    q.addBindValue(serializeMarks(marks));
    q.exec();
}

QByteArray PartyManager::serializeMarks(const QVector<QPair<double,double>> &marks) const
{
    QByteArray blob;
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    out << marks.size();
    for (auto &m : marks)
        out << m.first << m.second;

    return blob;
}

void PartyManager::addPressureSample(double timestamp, double pressure)
{
    pressureHistory.append({timestamp, pressure});
}

void PartyManager::addCameraPressureSample(int camera, double timestamp, double pressure)
{
    cameraPressureHistory[camera].append({timestamp, pressure});
}

QByteArray PartyManager::serializePressure() const
{
    QByteArray blob;
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    out << pressureHistory.size();
    for (auto &p : pressureHistory)
        out << p.first << p.second;

    return blob;
}

QByteArray PartyManager::serializeCameraPressure() const
{
    QByteArray blob;
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    out << cameraPressureHistory.size();

    for (auto it = cameraPressureHistory.begin(); it != cameraPressureHistory.end(); ++it) {
        out << it.key();            // номер камеры
        out << it.value().size();   // количество точек

        for (auto &p : it.value())
            out << p.first << p.second;
    }

    return blob;
}

void PartyManager::finalizeCurrentParty()
{
    if (currentPartyId < 0)
        return;

    QSqlQuery q;
    q.prepare("UPDATE parties SET end_time=datetime('now'), "
              "pressure_history = ?, "
              "camera_pressure_history = ? "
              "WHERE id=?");

    q.addBindValue(serializePressure());
    q.addBindValue(serializeCameraPressure());
    q.addBindValue(currentPartyId);

    q.exec();
}
