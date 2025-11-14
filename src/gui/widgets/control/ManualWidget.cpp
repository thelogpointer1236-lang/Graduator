#include "ManualWidget.h"

#include "core/services/ServiceLocator.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QMessageBox>
#include <QMetaObject>
#include <QSizePolicy>

ManualWidget::ManualWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    connectSignals();
}

ManualWidget::~ManualWidget() = default;

void ManualWidget::setupUi()
{
    auto *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(createDirectionGroup(), 0, 0);
    mainLayout->addWidget(createMotorGroup(), 1, 0);
    mainLayout->addWidget(createValveGroup(), 0, 1, 2, 1);
    setLayout(mainLayout);
}

QGroupBox *ManualWidget::createDirectionGroup()
{
    auto *group = new QGroupBox(tr("Engine direction"), this);
    auto *layout = new QHBoxLayout;
    m_radioForward = new QRadioButton(tr("Forward"), this);
    m_radioBackward = new QRadioButton(tr("Backward"), this);
    m_radioForward->setChecked(true);
    layout->addWidget(m_radioForward);
    layout->addWidget(m_radioBackward);
    group->setLayout(layout);
    return group;
}

QGroupBox *ManualWidget::createMotorGroup()
{
    auto *group = new QGroupBox(QString::fromWCharArray(L"Engine control"), this);
    auto *layout = new QVBoxLayout;
    m_btnStart = new QPushButton(tr("Start"), this);
    m_btnStopMotor = new QPushButton(tr("Stop"), this);
    layout->addWidget(m_btnStart);
    layout->addWidget(m_btnStopMotor);
    group->setLayout(layout);
    return group;
}

QGroupBox *ManualWidget::createValveGroup()
{
    auto *group = new QGroupBox(tr("Valve contol"), this);
    auto *layout = new QVBoxLayout;
    m_btnOpenInlet = new QPushButton(tr("Open inlet"), this);
    m_btnOpenOutlet = new QPushButton(tr("Open outlet"), this);
    m_btnCloseBoth = new QPushButton(tr("Close both"), this);
    for (auto *btn : {m_btnOpenInlet, m_btnOpenOutlet, m_btnCloseBoth}) {
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    layout->addWidget(m_btnOpenInlet);
    layout->addWidget(m_btnOpenOutlet);
    layout->addWidget(m_btnCloseBoth);
    group->setLayout(layout);
    return group;
}

void ManualWidget::connectSignals()
{
    connect(m_btnStart, &QPushButton::clicked, this, &ManualWidget::onStartMotor);
    connect(m_btnStopMotor, &QPushButton::clicked, this, &ManualWidget::onStopMotor);
    connect(m_btnOpenInlet, &QPushButton::clicked, this, &ManualWidget::onOpenInlet);
    connect(m_btnOpenOutlet, &QPushButton::clicked, this, &ManualWidget::onOpenOutlet);
    connect(m_btnCloseBoth, &QPushButton::clicked, this, &ManualWidget::onCloseBoth);
}

void ManualWidget::onStartMotor()
{
    auto *pressureController = ServiceLocator::instance().pressureController();
    if (pressureController->isRunning()) return;
    if (QString err; !pressureController->g540Driver()->isReadyToStart(err)) {
        QMessageBox::critical(
            this,
            tr("Failed to start motor"),
            err,
            tr("Ok")
        );
    }
    if (m_radioForward->isChecked())
        QMetaObject::invokeMethod(pressureController, "startGoToEnd", Qt::QueuedConnection);
    else
        QMetaObject::invokeMethod(pressureController, "startGoToStart", Qt::QueuedConnection);
    m_radioForward->setEnabled(false);
    m_radioBackward->setEnabled(false);
    m_btnStart->setEnabled(false);
    m_btnStopMotor->setEnabled(true);
}

void ManualWidget::onStopMotor()
{
    auto *pressureController = ServiceLocator::instance().pressureController();
    if (pressureController->isRunning()) {
        pressureController->stop();
    }
    m_radioForward->setEnabled(true);
    m_radioBackward->setEnabled(true);
    m_btnStart->setEnabled(true);
    m_btnStopMotor->setEnabled(false);
}

void ManualWidget::onOpenInlet() const
{
    ServiceLocator::instance().pressureController()->openInletFlap();
}

void ManualWidget::onOpenOutlet() const
{
    ServiceLocator::instance().pressureController()->openOutletFlap();
}

void ManualWidget::onCloseBoth() const
{
    ServiceLocator::instance().pressureController()->closeBothFlaps();
}
