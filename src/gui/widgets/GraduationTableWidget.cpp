#include "GraduationTableWidget.h"

#include "gui/models/GraduationTableModel.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>

GraduationTableWidget::GraduationTableWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void GraduationTableWidget::setupUi()
{
    auto *layout = createMainLayout();
    layout->addWidget(createTableView());
    setLayout(layout);
}

QVBoxLayout *GraduationTableWidget::createMainLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    return layout;
}

QTableView *GraduationTableWidget::createTableView()
{
    auto *tableView = new QTableView(this);
    tableView->setModel(new GraduationTableModel(this));
    tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    return tableView;
}
