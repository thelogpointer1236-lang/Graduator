#ifndef GRADUATOR_PARTYWIDGET_H
#define GRADUATOR_PARTYWIDGET_H

#include <QWidget>

class QCheckBox;
class QPushButton;
class QLabel;
class QRadioButton;
class QButtonGroup;

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

    void onSightModeSelected();
    void onDirectModeSelected();
    void onDirectReverseModeSelected();

private:
    class QHBoxLayout *createPartyHeader();
    class QCheckBox *createStrongNodeCheck();
    class QPushButton *createSaveButton();
    class QWidget *createGraduationModeSelector();

    void setupUi();
    void setupConnections();

private:
    QLabel *partyNumberLabel_ = nullptr;
    QLabel *savedPartyNumberLabel_ = nullptr;
    QCheckBox *strongNodeCheck_ = nullptr;
    QPushButton *saveButton_ = nullptr;

    QButtonGroup *modeGroup_ = nullptr;
    QRadioButton *sightModeRadio_ = nullptr;
    QRadioButton *directModeRadio_ = nullptr;
    QRadioButton *directReverseModeRadio_ = nullptr;
};

#endif // GRADUATOR_PARTYWIDGET_H
