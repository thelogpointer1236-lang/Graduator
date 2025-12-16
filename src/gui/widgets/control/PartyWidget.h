#ifndef GRADUATOR_PARTYWIDGET_H

#define GRADUATOR_PARTYWIDGET_H

#include <QWidget>

class QCheckBox;
class QPushButton;
class QLabel;

class PartyWidget final : public QWidget {
    Q_OBJECT

public:
    explicit PartyWidget(QWidget *parent = nullptr);
    ~PartyWidget() override;

private slots:
    void setPartyNumber(int number);
    void onSaveClicked();
    void onStrongNodeToggled(bool checked);
    void onResultAvailabilityChanged(bool available);
    void showSavedPartyNumber(int number);

private:
    class QHBoxLayout *createPartyHeader();
    class QCheckBox *createStrongNodeCheck();
    class QPushButton *createSaveButton();
    void setupUi();
    void setupConnections();

    QLabel *partyNumberLabel_ = nullptr;
    QLabel *savedPartyNumberLabel_ = nullptr;
    QCheckBox *strongNodeCheck_ = nullptr;
    QPushButton *saveButton_ = nullptr;
};

#endif // GRADUATOR_PARTYWIDGET_H
