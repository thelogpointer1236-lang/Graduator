#include "MainWindow.h"
#include "core/services/Logger.h"
#include "core/services/ServiceLocator.h"
#include "gui/widgets/MainWidget.h"
#include "gui/widgets/notifications/NotificationArea.h"

#include <QCloseEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVector>

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

    mainLayout->setContentsMargins(0, 0, 5, 0);
    mainLayout->setSpacing(0);

    setupNotifications();

    auto *userDialogService = ServiceLocator::instance().userDialogService();
    if (!userDialogService) throw std::runtime_error("UserDialogService is not available in ServiceLocator");
    connect(ServiceLocator::instance().userDialogService(), &UserDialogService::dialogRequested,
    this, &MainWindow::onDialogRequested);
}
void MainWindow::closeEvent(QCloseEvent *event) {
    ServiceLocator::instance().configManager()->save();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    // QMainWindow::resizeEvent(event);
    // if (notificationArea_) {
    //     notificationArea_->setGeometry(centralWidget_->rect());
    // }
}

void MainWindow::setupNotifications() {
    notificationArea_ = new NotificationArea(centralWidget_);
    notificationArea_->setGeometry(centralWidget_->rect());

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

void MainWindow::onDialogRequested(const QString &title, const QString &message, const QStringList &options, QString *response) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);

    QVector<QAbstractButton*> buttons;
    buttons.reserve(options.size());

    for (const auto &option : options) {
        buttons.push_back(msgBox.addButton(option, QMessageBox::ActionRole));
    }

    msgBox.exec();

    if (auto *clicked = msgBox.clickedButton()) {
        const int index = buttons.indexOf(clicked);
        if (index >= 0 && index < options.size()) {
            *response = options.at(index);
            return;
        }
        *response = clicked->text();
        return;
    }

    *response = QString();
}
