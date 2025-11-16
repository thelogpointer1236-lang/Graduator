#include "PartyResultDialog.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QLabel>
#include <QAbstractItemView>
#include <QStringList>
#include <algorithm>

PartyResultDialog::PartyResultDialog(const PartyHistoryRecord &record, const PartyResult &result, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Party #%1").arg(record.partyNumber));
    resize(700, 500);

    auto *layout = new QVBoxLayout(this);
    auto *infoLabel = new QLabel(tr("Start: %1\nEnd: %2")
                                    .arg(record.startTime.isValid() ? record.startTime.toString(Qt::ISODate) : QStringLiteral("-"))
                                    .arg(record.endTime.isValid() ? record.endTime.toString(Qt::ISODate) : QStringLiteral("-")),
                                this);
    layout->addWidget(infoLabel);

    auto *tabs = new QTabWidget(this);
    auto *forwardTable = createResultTable(result.forward);
    auto *backwardTable = createResultTable(result.backward);
    tabs->addTab(forwardTable, tr("Forward"));
    tabs->addTab(backwardTable, tr("Backward"));
    layout->addWidget(tabs);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PartyResultDialog::reject);
    layout->addWidget(buttonBox);
}

QTableWidget *PartyResultDialog::createResultTable(const std::vector<std::vector<double>> &data) const
{
    auto *table = new QTableWidget;
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    populateTable(table, data);
    return table;
}

void PartyResultDialog::populateTable(QTableWidget *table, const std::vector<std::vector<double>> &data) const
{
    int rowCount = static_cast<int>(data.size());
    int columnCount = 0;
    for (const auto &row : data) {
        columnCount = std::max(columnCount, static_cast<int>(row.size()));
    }
    table->setRowCount(rowCount);
    table->setColumnCount(columnCount);

    QStringList headers;
    for (int column = 0; column < columnCount; ++column) {
        headers << tr("Cam %1").arg(column + 1);
    }
    table->setHorizontalHeaderLabels(headers);

    for (int row = 0; row < rowCount; ++row) {
        table->setVerticalHeaderItem(row, new QTableWidgetItem(tr("P%1").arg(row + 1)));
        const auto &values = data[row];
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(QString::number(values[column], 'f', 3));
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, column, item);
        }
    }
}
