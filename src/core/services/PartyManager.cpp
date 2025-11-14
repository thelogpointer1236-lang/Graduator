#include "PartyManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>
#include <QDebug>
PartyManager::PartyManager(QObject *parent) : QObject(parent) {
}
bool PartyManager::initDatabase(const QString &path) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) return false;
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS parties ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "party_number INTEGER,"
        "start_time TEXT,"
        "end_time TEXT)");
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
void PartyManager::updatePartyNumberIfNewDay() {
    currentDate = QDate::currentDate();
    QSqlQuery q;
    q.prepare("SELECT MAX(party_number) FROM parties WHERE date(start_time)=?");
    q.addBindValue(currentDate.toString(Qt::ISODate));
    if (q.exec() && q.next() && !q.value(0).isNull())
        partyNumber = q.value(0).toInt() + 1;
    else
        partyNumber = 1;
}
void PartyManager::loadOrCreateCurrentParty() {
    QSqlQuery q;
    q.prepare("INSERT INTO parties (party_number, start_time) VALUES (?, datetime('now'))");
    q.addBindValue(partyNumber);
    q.exec();
    currentPartyId = q.lastInsertId().toInt();
}
void PartyManager::startNewMeasurement() {
    partyNumber++;
    createNewParty();
}
void PartyManager::createNewParty() {
    QSqlQuery q;
    q.prepare("INSERT INTO parties (party_number, start_time) VALUES (?, datetime('now'))");
    q.addBindValue(partyNumber);
    q.exec();
    currentPartyId = q.lastInsertId().toInt();
    emit partyNumberChanged(partyNumber);
}
void PartyManager::addGauge(const QString &name, const QVector<QPair<double, double> > &marks) {
    if (currentPartyId < 0) return;
    QSqlQuery q;
    q.prepare("INSERT INTO gauges (party_id, name, marks) VALUES (?, ?, ?)");
    q.addBindValue(currentPartyId);
    q.addBindValue(name);
    q.addBindValue(serializeMarks(marks));
    q.exec();
}
QByteArray PartyManager::serializeMarks(const QVector<QPair<double, double> > &marks) const {
    QByteArray blob;
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);
    out << marks.size();
    for (const auto &m: marks)
        out << m.first << m.second;
    return blob;
}
QVector<QPair<double, double> > PartyManager::deserializeMarks(const QByteArray &blob) const {
    QVector<QPair<double, double> > marks;
    QDataStream in(blob);
    in.setVersion(QDataStream::Qt_5_0);
    int size = 0;
    in >> size;
    marks.reserve(size);
    for (int i = 0; i < size; ++i) {
        double p, a;
        in >> p >> a;
        marks.append({p, a});
    }
    return marks;
}
void PartyManager::finalizeCurrentParty() {
    if (currentPartyId < 0) return;
    QSqlQuery q;
    q.prepare("UPDATE parties SET end_time=datetime('now') WHERE id=?");
    q.addBindValue(currentPartyId);
    q.exec();
}
int PartyManager::currentPartyNumber() const {
    return partyNumber;
}