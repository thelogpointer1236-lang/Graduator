#ifndef GRADUATOR_STATUSBARMODEL_H
#define GRADUATOR_STATUSBARMODEL_H

#include <QAbstractItemModel>
#include <QObject>
#include <QColor>
#include <QFont>
#include <QTimer>
#include <QElapsedTimer>

#include "core/types/PressureUnit.h"

class StatusBarModel final : public QAbstractItemModel {
    Q_OBJECT

    static inline constexpr int ColumnCount = 8;

    QTimer m_updateTimer;
    QElapsedTimer m_elapsed;

public:
    explicit StatusBarModel(QObject *parent = nullptr);

    QModelIndex index(int row, int col, const QModelIndex &) const override;
    QModelIndex parent(const QModelIndex &) const override;

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;

    QVariant data(const QModelIndex &idx, int role) const override;

private:
    static QString pressureUnitToString(PressureUnit unit);
    QString formatTime(qint64 ms) const;

};

#endif // GRADUATOR_STATUSBARMODEL_H
