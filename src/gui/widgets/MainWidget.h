#ifndef GRADUATOR_MAINWIDGET_H
#define GRADUATOR_MAINWIDGET_H

#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QTabWidget;

class MainWidget final : public QWidget {
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = nullptr);
private:
    void setupUi();
    void setupLeftPanel(QHBoxLayout *mainLayout);
    void setupRightPanel(QVBoxLayout *rightLayout);
    QTabWidget* createTabWidget();
    QWidget* createControlPage();
};

#endif // GRADUATOR_MAINWIDGET_H
