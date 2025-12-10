#include "PartyWidget.h"

#include "core/services/ServiceLocator.h"
#include "core/types/PartyResult.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QFont>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>

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
    frameLayout->setSpacing(5);
    frameLayout->addLayout(createPartyHeader());
    frameLayout->addWidget(createStrongNodeCheck());
    frameLayout->addWidget(createSaveButton());
    frameLayout->addStretch();

    mainLayout->addWidget(frame);
}

void PartyWidget::setupConnections()
{
    connect(saveButton_, &QPushButton::clicked, this, &PartyWidget::onSaveClicked);
    connect(strongNodeCheck_, &QCheckBox::toggled, this, &PartyWidget::onStrongNodeToggled);
    connect(ServiceLocator::instance().partyManager(), &PartyManager::partyNumberChanged,
            this, &PartyWidget::setPartyNumber);
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
    auto *partyLabelText = new QLabel(tr("Number of party") + ": ", this);
    partyNumberLabel_ = new QLabel(QString::number(ServiceLocator::instance().partyManager()->partyNumber()), this);
    QFont font = partyNumberLabel_->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    partyNumberLabel_->setFont(font);
    partyLabelText->setFont(font);
    layout->addWidget(partyLabelText);
    layout->addWidget(partyNumberLabel_);
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
    const PartyResult result = graduationService->getPartyResult();
    if (QString err; !partyManager->savePartyResult(result, err)) {
        QMessageBox::critical(this, tr("Saving"), tr("Failed to store the graduation result for the current party.") + "\n" + err);
        return;
    }
    QMessageBox::information(this, tr("Saving"), tr("The graduation result has been saved."));
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

void PartyWidget::onResultAvailabilityChanged(bool available)
{
    if (saveButton_) {
        saveButton_->setEnabled(available);
    }
}
