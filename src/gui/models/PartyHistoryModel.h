#ifndef GRADUATOR_PARTYHISTORYMODEL_H
#define GRADUATOR_PARTYHISTORYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "core/services/PartyManager.h"

class PartyHistoryModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit PartyHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    PartyHistoryRecord recordAt(int row) const;

public slots:
    void reload();

private:
    QVector<PartyHistoryRecord> m_records;
};

#endif // GRADUATOR_PARTYHISTORYMODEL_H
