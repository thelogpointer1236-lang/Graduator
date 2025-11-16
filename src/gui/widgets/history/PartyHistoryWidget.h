#ifndef GRADUATOR_PARTYHISTORYWIDGET_H
#define GRADUATOR_PARTYHISTORYWIDGET_H

#include <QWidget>
#include "core/services/PartyManager.h"
#include "core/types/PartyResult.h"

class QTableView;
class QPushButton;
class PartyHistoryModel;
class QModelIndex;

class PartyHistoryWidget final : public QWidget {
    Q_OBJECT
public:
    explicit PartyHistoryWidget(QWidget *parent = nullptr);

private slots:
    void onViewRequested();
    void onApplyRequested();
    void updateButtonsState();
    void onRowDoubleClicked(const QModelIndex &index);

private:
    void setupUi();
    void setupConnections();
    PartyHistoryRecord selectedRecord() const;
    bool loadResultForSelection(PartyResult &result, PartyHistoryRecord &record);

    QTableView *tableView_ = nullptr;
    QPushButton *viewButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
    PartyHistoryModel *model_ = nullptr;
};

#endif // GRADUATOR_PARTYHISTORYWIDGET_H
