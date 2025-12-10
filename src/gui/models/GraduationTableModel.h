#ifndef GRADUATOR_GRADUATIONTABLEMODEL_H
#define GRADUATOR_GRADUATIONTABLEMODEL_H
#include <QAbstractTableModel>
#include <QTimer>

#include "core/types/Grad.h"
#include "core/types/PartyResult.h"

class GraduationTableModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit GraduationTableModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
private slots:
    void updateIndicators();
    void updateScale();
private:
    const class GaugeModel *m_gaugeModel;
    QString m_cameraStr;
    QTimer m_updateTimer;
    PartyResult m_partyResult;
    PartyValidationResult m_validationResult;
};
#endif //GRADUATOR_GRADUATIONTABLEMODEL_H
