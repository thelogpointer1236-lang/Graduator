#include "MainWindow.h"
#include "gui/widgets/MainWidget.h"
#include "core/services/ServiceLocator.h"
#include <QVBoxLayout>
#include <QCloseEvent>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Graduator"));
    // Создаем центральный виджет
    const auto centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    // Создаем ваш основной виджет
    auto *mainWidget = new MainWidget(this);
    // Устанавливаем layout для центрального виджета
    const auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainWidget);
    // Не забудьте установить layout в центральный виджет!
    centralWidget->setLayout(mainLayout);
}
void MainWindow::closeEvent(QCloseEvent *event) {
    ServiceLocator::instance().configManager()->save();
    event->accept();
}