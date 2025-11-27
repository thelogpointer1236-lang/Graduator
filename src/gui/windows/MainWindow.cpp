#include "MainWindow.h"
#include "core/services/Logger.h"
#include "core/services/ServiceLocator.h"
#include "gui/widgets/MainWidget.h"
#include "gui/widgets/notifications/NotificationArea.h"

#include <QCloseEvent>
#include <QResizeEvent>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Graduator"));
    // Создаем центральный виджет
    centralWidget_ = new QWidget(this);
    this->setCentralWidget(centralWidget_);
    // Создаем ваш основной виджет
    auto *mainWidget = new MainWidget(this);
    // Устанавливаем layout для центрального виджета
    const auto mainLayout = new QVBoxLayout(centralWidget_);
    mainLayout->addWidget(mainWidget);
    // Не забудьте установить layout в центральный виджет!
    centralWidget_->setLayout(mainLayout);

    setupNotifications();
}
void MainWindow::closeEvent(QCloseEvent *event) {
    ServiceLocator::instance().configManager()->save();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (notificationArea_) {
        notificationArea_->reposition();
    }
}

void MainWindow::setupNotifications() {
    notificationArea_ = new NotificationArea(centralWidget_);
    notificationArea_->reposition();

    if (auto *logger = ServiceLocator::instance().logger()) {
        connect(logger, &Logger::newLogEntry, this, [this](LogLevel level, const QString &text) {
            if (!notificationArea_) {
                return;
            }

            if (level == LogLevel::Warning || level == LogLevel::Error || level == LogLevel::Critical) {
                notificationArea_->showMessage(level, text.trimmed());
            }
        });
    }
}
