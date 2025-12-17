#ifndef GRADUATOR_CONTROLWIDGET_H
#define GRADUATOR_CONTROLWIDGET_H

#include <QWidget>

class QTabWidget;

class ControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ControlWidget(QWidget *parent = nullptr);
    ~ControlWidget() override;

public slots:
    void lockTabs(bool locked);

private:
    void setupUi();
    void setupTabs();
    void setupConnections();

    QTabWidget *m_tabWidget = nullptr;
};

#endif // GRADUATOR_CONTROLWIDGET_H
