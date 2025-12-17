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
    // Запрет выделения
    tableView->setSelectionMode(QAbstractItemView::NoSelection);

    // Запрет редактирования
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Убрать фокус (включая клики мышью/клавиатуру)
    tableView->setFocusPolicy(Qt::NoFocus);

    // Отключить сортировку и кликабельность заголовков
    tableView->setSortingEnabled(false);
    tableView->horizontalHeader()->setSectionsClickable(false);
    tableView->verticalHeader()->setSectionsClickable(false);

    // Отключить контекстное меню
    tableView->setContextMenuPolicy(Qt::NoContextMenu);
    tableView->viewport()->setAttribute(Qt::WA_TransparentForMouseEvents);
    return tableView;
}
