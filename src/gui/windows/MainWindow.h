#ifndef GRADUATOR_MAINWINDOW_H
#define GRADUATOR_MAINWINDOW_H
#include <QMainWindow>
class QCloseEvent;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif //GRADUATOR_MAINWINDOW_H