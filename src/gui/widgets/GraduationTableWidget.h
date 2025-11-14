#ifndef GRADUATOR_GRADUATIONTABLE_H

#define GRADUATOR_GRADUATIONTABLE_H

#include <QWidget>

class QVBoxLayout;
class QTableView;

class GraduationTableWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GraduationTableWidget(QWidget *parent = nullptr);

private:
    void setupUi();
    QVBoxLayout *createMainLayout();
    QTableView *createTableView();
};

#endif // GRADUATOR_GRADUATIONTABLE_H
