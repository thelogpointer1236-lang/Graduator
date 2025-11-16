#include "PartyHistoryWidget.h"

#include "gui/models/PartyHistoryModel.h"
#include "gui/dialogs/PartyResultDialog.h"
#include "core/services/ServiceLocator.h"

#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QAbstractItemView>

PartyHistoryWidget::PartyHistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
    updateButtonsState();
}

void PartyHistoryWidget::setupUi()
{
    model_ = new PartyHistoryModel(this);
    tableView_ = new QTableView(this);
    tableView_->setModel(model_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->setAlternatingRowColors(true);

    viewButton_ = new QPushButton(tr("View"), this);
    applyButton_ = new QPushButton(tr("Apply"), this);

    auto *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(viewButton_);
    buttonsLayout->addWidget(applyButton_);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tableView_);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

void PartyHistoryWidget::setupConnections()
{
    connect(viewButton_, &QPushButton::clicked, this, &PartyHistoryWidget::onViewRequested);
    connect(applyButton_, &QPushButton::clicked, this, &PartyHistoryWidget::onApplyRequested);
    connect(tableView_, &QTableView::doubleClicked, this, &PartyHistoryWidget::onRowDoubleClicked);
    if (tableView_->selectionModel()) {
        connect(tableView_->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &PartyHistoryWidget::updateButtonsState);
    }
    connect(model_, &QAbstractItemModel::modelReset, this, &PartyHistoryWidget::updateButtonsState);
}

PartyHistoryRecord PartyHistoryWidget::selectedRecord() const
{
    if (!tableView_ || !tableView_->selectionModel()) {
        return {};
    }
    const auto rows = tableView_->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    return model_->recordAt(rows.first().row());
}

bool PartyHistoryWidget::loadResultForSelection(PartyResult &result, PartyHistoryRecord &record)
{
    record = selectedRecord();
    if (!record.isValid()) {
        QMessageBox::information(this, tr("Parties"), tr("Select a party from the list."));
        return false;
    }
    if (!record.hasResult) {
        QMessageBox::warning(this, tr("Parties"), tr("The selected party does not contain a saved result."));
        return false;
    }
    auto *manager = ServiceLocator::instance().partyManager();
    if (!manager) {
        QMessageBox::critical(this, tr("Parties"), tr("Party manager is not available."));
        return false;
    }
    result = manager->loadPartyResult(record.id);
    if (!result.isValid()) {
        QMessageBox::warning(this, tr("Parties"), tr("Failed to load the stored result for the selected party."));
        return false;
    }
    return true;
}

void PartyHistoryWidget::onViewRequested()
{
    PartyResult result;
    PartyHistoryRecord record;
    if (!loadResultForSelection(result, record)) {
        return;
    }
    PartyResultDialog dialog(record, result, this);
    dialog.exec();
}

void PartyHistoryWidget::onApplyRequested()
{
    PartyResult result;
    PartyHistoryRecord record;
    if (!loadResultForSelection(result, record)) {
        return;
    }
    if (auto *service = ServiceLocator::instance().graduationService()) {
        service->setResult(result);
        QMessageBox::information(this, tr("Parties"), tr("The result has been applied."));
    } else {
        QMessageBox::critical(this, tr("Parties"), tr("Graduation service is not available."));
    }
}

void PartyHistoryWidget::updateButtonsState()
{
    const PartyHistoryRecord record = selectedRecord();
    const bool hasSelection = record.isValid() && record.hasResult;
    viewButton_->setEnabled(hasSelection);
    applyButton_->setEnabled(hasSelection);
}

void PartyHistoryWidget::onRowDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        onViewRequested();
    }
}
