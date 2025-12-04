#ifndef GRADUATOR_MAINWINDOW_H
#define GRADUATOR_MAINWINDOW_H
#include <QMainWindow>

class QCloseEvent;
class NotificationArea;
class QWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupNotifications();

private slots:
    void onUserConfirmationRequested(int* resp);

private:
    QWidget *centralWidget_{};
    NotificationArea *notificationArea_{};
};
#endif //GRADUATOR_MAINWINDOW_H
