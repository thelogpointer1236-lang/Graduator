#include "PartyWidget.h"

#include "core/services/ServiceLocator.h"
#include "core/types/PartyResult.h"

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QFont>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QRadioButton>

PartyWidget::PartyWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
}

PartyWidget::~PartyWidget() = default;

void PartyWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 15, 0);
    mainLayout->setSpacing(0);

    auto *frame = new QFrame(this);
    frame->setFrameShape(QFrame::Box);
    frame->setFrameShadow(QFrame::Raised);
    frame->setLineWidth(1);

    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setSpacing(6);

    frameLayout->addLayout(createPartyHeader());
    frameLayout->addWidget(createGraduationModeSelector());
    frameLayout->addWidget(createStrongNodeCheck());
    frameLayout->addWidget(createSaveButton());
    frameLayout->addStretch();

    mainLayout->addWidget(frame);
}

QWidget *PartyWidget::createGraduationModeSelector()
{
    auto *groupBox = new QGroupBox(tr("Graduation mode"), this);

    sightModeRadio_ = new QRadioButton(tr("Sight"), groupBox);
    directModeRadio_ = new QRadioButton(tr("Direct stroke"), groupBox);
    directReverseModeRadio_ = new QRadioButton(tr("Direct and reverse stroke"), groupBox);

    modeGroup_ = new QButtonGroup(this);
    modeGroup_->addButton(sightModeRadio_);
    modeGroup_->addButton(directModeRadio_);
    modeGroup_->addButton(directReverseModeRadio_);

    sightModeRadio_->setChecked(true);

    auto *layout = new QVBoxLayout(groupBox);
    layout->addWidget(sightModeRadio_);
    layout->addWidget(directModeRadio_);
    layout->addWidget(directReverseModeRadio_);
    layout->addStretch();

    return groupBox;
}

void PartyWidget::setupConnections()
{
    connect(saveButton_, &QPushButton::clicked,
            this, &PartyWidget::onSaveClicked);
    connect(strongNodeCheck_, &QCheckBox::toggled,
            this, &PartyWidget::onStrongNodeToggled);

    connect(ServiceLocator::instance().partyManager(),
            &PartyManager::partyNumberChanged,
            this, &PartyWidget::setPartyNumber);

    connect(sightModeRadio_, &QRadioButton::toggled, this,
            [this](bool checked) { if (checked) onSightModeSelected(); });
    connect(directModeRadio_, &QRadioButton::toggled, this,
            [this](bool checked) { if (checked) onDirectModeSelected(); });
    connect(directReverseModeRadio_, &QRadioButton::toggled, this,
            [this](bool checked) { if (checked) onDirectReverseModeSelected(); });

    saveButton_->setEnabled(false);

    if (auto *graduationService = ServiceLocator::instance().graduationService()) {
        connect(graduationService, &GraduationService::resultAvailabilityChanged,
                this, &PartyWidget::onResultAvailabilityChanged);
    }
}

QHBoxLayout *PartyWidget::createPartyHeader()
{
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    auto *labelsLayout = new QVBoxLayout;
    labelsLayout->setContentsMargins(0, 0, 0, 0);

    auto *currentPartyLayout = new QHBoxLayout;
    currentPartyLayout->setContentsMargins(0, 0, 0, 0);
    auto *partyLabelText = new QLabel(tr("Number of party") + ": ", this);
    partyNumberLabel_ = new QLabel(QString::number(ServiceLocator::instance().partyManager()->partyNumber()), this);
    QFont font = partyNumberLabel_->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    partyNumberLabel_->setFont(font);
    partyLabelText->setFont(font);
    currentPartyLayout->addWidget(partyLabelText);
    currentPartyLayout->addWidget(partyNumberLabel_);
    currentPartyLayout->addStretch();

    savedPartyNumberLabel_ = new QLabel(this);
    savedPartyNumberLabel_->setFont(font);
    savedPartyNumberLabel_->setStyleSheet(QStringLiteral("color: green;"));
    savedPartyNumberLabel_->setVisible(false);

    labelsLayout->addLayout(currentPartyLayout);
    labelsLayout->addWidget(savedPartyNumberLabel_);

    layout->addLayout(labelsLayout);
    layout->addStretch();
    return layout;
}

QCheckBox *PartyWidget::createStrongNodeCheck()
{
    strongNodeCheck_ = new QCheckBox(tr("Strong mark"), this);
    return strongNodeCheck_;
}

QPushButton *PartyWidget::createSaveButton()
{
    saveButton_ = new QPushButton(tr("Save"), this);
    return saveButton_;
}

void PartyWidget::onSaveClicked()
{
    auto *graduationService = ServiceLocator::instance().graduationService();
    if (!graduationService || !graduationService->isResultReady()) {
        QMessageBox::warning(this, tr("Saving"), tr("No valid graduation result is available for saving."));
        return;
    }
    auto *partyManager = ServiceLocator::instance().partyManager();
    const int savedPartyNumber = partyManager->partyNumber();
    const PartyResult result = graduationService->getPartyResult();
    if (partyManager->savePartyResult(result)) {
        showSavedPartyNumber(savedPartyNumber);
    }
}

void PartyWidget::onStrongNodeToggled(bool checked)
{
    if (auto *graduationService = ServiceLocator::instance().graduationService()) {
        graduationService->setStrongNode(checked);
        if (graduationService->isResultReady()) {
            graduationService->requestUpdateResultAndTable();
        }
    }
}

void PartyWidget::setPartyNumber(int number)
{
    if (partyNumberLabel_) {
        partyNumberLabel_->setText(QString::number(number));
    }
}

void PartyWidget::showSavedPartyNumber(int number)
{
    if (savedPartyNumberLabel_) {
        savedPartyNumberLabel_->setText(tr("Saved party number") + ": " + QString::number(number));
        savedPartyNumberLabel_->setVisible(true);
    }
}

void PartyWidget::onSightModeSelected() {
    ServiceLocator::instance().pressureController()->setMode(PressureControllerMode::Inference);
}

void PartyWidget::onDirectModeSelected() {
    ServiceLocator::instance().pressureController()->setMode(PressureControllerMode::Forward);
}

void PartyWidget::onDirectReverseModeSelected() {
    ServiceLocator::instance().pressureController()->setMode(PressureControllerMode::ForwardBackward);
}

void PartyWidget::onResultAvailabilityChanged(bool available)
{
    if (saveButton_) {
        saveButton_->setEnabled(available);
    }
}
