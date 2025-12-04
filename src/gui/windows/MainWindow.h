#ifndef GRADUATOR_MAINWINDOW_H
#define GRADUATOR_MAINWINDOW_H
#include <QMainWindow>
#include <QStringList>

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
    void onDialogRequested(const QString &title, const QString &message, const QStringList &options, QString *response);

private:
    QWidget *centralWidget_{};
    NotificationArea *notificationArea_{};
};
#endif //GRADUATOR_MAINWINDOW_H
