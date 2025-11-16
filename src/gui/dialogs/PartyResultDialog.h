#ifndef GRADUATOR_PARTYRESULTDIALOG_H
#define GRADUATOR_PARTYRESULTDIALOG_H

#include <QDialog>
#include "core/services/PartyManager.h"
#include "core/types/PartyResult.h"

class QTableWidget;

class PartyResultDialog final : public QDialog {
    Q_OBJECT
public:
    PartyResultDialog(const PartyHistoryRecord &record, const PartyResult &result, QWidget *parent = nullptr);

private:
    QTableWidget *createResultTable(const std::vector<std::vector<double>> &data) const;
    void populateTable(QTableWidget *table, const std::vector<std::vector<double>> &data) const;
};

#endif // GRADUATOR_PARTYRESULTDIALOG_H
