#include "ControlWidget.h"

#include "AutomaticWidget.h"
#include "ManualWidget.h"
#include "PartyWidget.h"
#include "core/services/ServiceLocator.h"

#include <QHBoxLayout>
#include <QTabWidget>
#include <QTabBar>

namespace {
constexpr int PartyPanelStretch = 1;
constexpr int TabsStretch = 1;
}

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
}

ControlWidget::~ControlWidget() = default;

void ControlWidget::setupUi()
{
    setupTabs();
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    auto *partyWidget = new PartyWidget(this);
    mainLayout->addWidget(partyWidget, PartyPanelStretch);
    mainLayout->addWidget(m_tabWidget, TabsStretch);
    setLayout(mainLayout);
}

void ControlWidget::setupTabs()
{
    auto *automaticWidget = new AutomaticWidget(this);
    auto *manualWidget = new ManualWidget(this);
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(automaticWidget, tr("Automatic Mode"));
    m_tabWidget->addTab(manualWidget, tr("Manual Mode"));
}

void ControlWidget::setupConnections()
{
    auto *pressureController = ServiceLocator::instance().pressureController();
    connect(pressureController, &PressureControllerBase::started, this, [this] { lockTabs(true); });
    connect(pressureController, &PressureControllerBase::interrupted, this, [this] { lockTabs(false); });
    connect(pressureController, &PressureControllerBase::resultReady, this, [this] { lockTabs(false); });
}

void ControlWidget::lockTabs(bool locked)
{
    if (m_tabWidget) {
        m_tabWidget->tabBar()->setEnabled(!locked);
    }
}
