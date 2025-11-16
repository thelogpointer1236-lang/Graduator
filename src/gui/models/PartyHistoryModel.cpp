#include "PartyHistoryModel.h"

#include <QLocale>
#include <QDateTime>
#include "core/services/ServiceLocator.h"

namespace {
constexpr int ColumnCount = 4;
constexpr int ColumnPartyNumber = 0;
constexpr int ColumnStart = 1;
constexpr int ColumnEnd = 2;
constexpr int ColumnResult = 3;

QString formatDateTime(const QDateTime &dt)
{
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return QLocale().toString(dt, QLocale::ShortFormat);
}
}

PartyHistoryModel::PartyHistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    reload();
    if (auto *manager = ServiceLocator::instance().partyManager()) {
        connect(manager, &PartyManager::historyChanged, this, &PartyHistoryModel::reload);
    }
}

int PartyHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_records.size();
}

int PartyHistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant PartyHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size()) {
        return {};
    }
    const auto &record = m_records.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColumnPartyNumber:
            return record.partyNumber;
        case ColumnStart:
            return formatDateTime(record.startTime);
        case ColumnEnd:
            return formatDateTime(record.endTime);
        case ColumnResult:
            return record.hasResult ? tr("Yes") : tr("No");
        default:
            break;
        }
    }
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColumnPartyNumber || index.column() == ColumnResult) {
            return Qt::AlignCenter;
        }
    }
    return {};
}

QVariant PartyHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColumnPartyNumber:
            return tr("Party");
        case ColumnStart:
            return tr("Start time");
        case ColumnEnd:
            return tr("End time");
        case ColumnResult:
            return tr("Result saved");
        default:
            break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

PartyHistoryRecord PartyHistoryModel::recordAt(int row) const
{
    if (row < 0 || row >= m_records.size()) {
        return {};
    }
    return m_records.at(row);
}

void PartyHistoryModel::reload()
{
    beginResetModel();
    if (auto *manager = ServiceLocator::instance().partyManager()) {
        m_records = manager->partyHistory();
    } else {
        m_records.clear();
    }
    endResetModel();
}
