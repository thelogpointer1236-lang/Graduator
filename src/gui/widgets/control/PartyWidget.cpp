#include "PartyWidget.h"

#include "core/services/ServiceLocator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QFont>
#include <QCheckBox>
#include <QPushButton>

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
    frameLayout->addWidget(createAlignTopCheck());
    frameLayout->addWidget(createStrongKnotCheck());
    frameLayout->addWidget(createSaveButton());
    frameLayout->addStretch();

    mainLayout->addWidget(frame);
}

void PartyWidget::setupConnections()
{
    connect(saveButton_, &QPushButton::clicked, this, &PartyWidget::onSaveClicked);
    connect(alignTopCheck_, &QCheckBox::toggled, this, &PartyWidget::onAlignTopNodeToggled);
    connect(strongKnotCheck_, &QCheckBox::toggled, this, &PartyWidget::onStrongNodeToggled);
    connect(ServiceLocator::instance().partyManager(), &PartyManager::partyNumberChanged,
            this, &PartyWidget::setPartyNumber);
}

QHBoxLayout *PartyWidget::createPartyHeader()
{
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);
    auto *partyLabelText = new QLabel(tr("Number of party") + ": ", this);
    partyNumberLabel_ = new QLabel(QString::fromWCharArray(L"1"), this);
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

QCheckBox *PartyWidget::createAlignTopCheck()
{
    alignTopCheck_ = new QCheckBox(tr("Center top mark"), this);
    return alignTopCheck_;
}

QCheckBox *PartyWidget::createStrongKnotCheck()
{
    strongKnotCheck_ = new QCheckBox(tr("Strong mark"), this);
    return strongKnotCheck_;
}

QPushButton *PartyWidget::createSaveButton()
{
    saveButton_ = new QPushButton(tr("Save"), this);
    return saveButton_;
}

void PartyWidget::onSaveClicked()
{
}

void PartyWidget::onAlignTopNodeToggled(bool checked)
{
}

void PartyWidget::onStrongNodeToggled(bool checked)
{
}

void PartyWidget::setPartyNumber(int number)
{
    if (partyNumberLabel_) {
        partyNumberLabel_->setText(QString::number(number));
    }
}
