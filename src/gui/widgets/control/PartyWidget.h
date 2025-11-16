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
    void onAlignTopNodeToggled(bool checked);
    void onStrongNodeToggled(bool checked);
    void onResultAvailabilityChanged(bool available);

private:
    class QHBoxLayout *createPartyHeader();
    class QCheckBox *createAlignTopCheck();
    class QCheckBox *createStrongKnotCheck();
    class QPushButton *createSaveButton();
    void setupUi();
    void setupConnections();

    QLabel *partyNumberLabel_ = nullptr;
    QCheckBox *alignTopCheck_ = nullptr;
    QCheckBox *strongKnotCheck_ = nullptr;
    QPushButton *saveButton_ = nullptr;
};

#endif // GRADUATOR_PARTYWIDGET_H
