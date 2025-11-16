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

    ensureResultColumns();

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
    emit historyChanged();
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
    emit historyChanged();
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
    emit historyChanged();
}

void PartyManager::ensureResultColumns()
{
    QSqlQuery pragma("PRAGMA table_info(parties)");
    bool hasForward = false;
    bool hasBackward = false;
    while (pragma.next()) {
        const auto columnName = pragma.value("name").toString();
        if (columnName == "result_forward") {
            hasForward = true;
        } else if (columnName == "result_backward") {
            hasBackward = true;
        }
    }
    if (!hasForward) {
        QSqlQuery q;
        q.exec("ALTER TABLE parties ADD COLUMN result_forward BLOB");
    }
    if (!hasBackward) {
        QSqlQuery q;
        q.exec("ALTER TABLE parties ADD COLUMN result_backward BLOB");
    }
}

QByteArray PartyManager::serializeResult(const std::vector<std::vector<double>> &result) const
{
    QByteArray blob;
    if (result.empty()) {
        return blob;
    }
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);
    out << static_cast<quint32>(result.size());
    for (const auto &row : result) {
        out << static_cast<quint32>(row.size());
        for (double value : row) {
            out << value;
        }
    }
    return blob;
}

std::vector<std::vector<double>> PartyManager::deserializeResult(const QByteArray &blob) const
{
    std::vector<std::vector<double>> result;
    if (blob.isEmpty()) {
        return result;
    }
    QDataStream in(blob);
    in.setVersion(QDataStream::Qt_5_0);
    quint32 rows = 0;
    in >> rows;
    result.resize(rows);
    for (quint32 row = 0; row < rows; ++row) {
        quint32 columns = 0;
        in >> columns;
        result[row].resize(columns);
        for (quint32 column = 0; column < columns; ++column) {
            double value = 0.0;
            in >> value;
            result[row][column] = value;
        }
    }
    return result;
}

QVector<PartyHistoryRecord> PartyManager::partyHistory() const
{
    QVector<PartyHistoryRecord> records;
    QSqlQuery q;
    q.prepare("SELECT id, party_number, start_time, end_time, result_forward, result_backward "
              "FROM parties ORDER BY id DESC");
    if (!q.exec()) {
        return records;
    }
    while (q.next()) {
        PartyHistoryRecord record;
        record.id = q.value(0).toInt();
        record.partyNumber = q.value(1).toInt();
        record.startTime = QDateTime::fromString(q.value(2).toString(), Qt::ISODate);
        record.endTime = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
        record.hasResult = !q.value(4).toByteArray().isEmpty() || !q.value(5).toByteArray().isEmpty();
        records.push_back(record);
    }
    return records;
}

PartyResult PartyManager::loadPartyResult(int partyId) const
{
    PartyResult result;
    QSqlQuery q;
    q.prepare("SELECT result_forward, result_backward FROM parties WHERE id = ?");
    q.addBindValue(partyId);
    if (!q.exec()) {
        return result;
    }
    if (q.next()) {
        result.forward = deserializeResult(q.value(0).toByteArray());
        result.backward = deserializeResult(q.value(1).toByteArray());
    }
    return result;
}

bool PartyManager::saveCurrentPartyResult(const PartyResult &result)
{
    if (currentPartyId < 0 || !result.isValid()) {
        return false;
    }
    QSqlQuery q;
    q.prepare("UPDATE parties SET result_forward = ?, result_backward = ? WHERE id = ?");
    q.addBindValue(serializeResult(result.forward));
    q.addBindValue(serializeResult(result.backward));
    q.addBindValue(currentPartyId);
    const bool ok = q.exec();
    if (ok) {
        emit historyChanged();
    }
    return ok;
}
