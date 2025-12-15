#include "MainWidget.h"
#include "cameraGrid/CameraGridWidget.h"
#include "control/ControlWidget.h"
#include "GraduationTableWidget.h"
#include "gui/views/StatusBarView.h"
#include "gui/models/StatusBarModel.h"
#include "SettingsWidget.h"
#include "CameraSettingsTab.h"
#include "LogAndConfigTab.h"
#include <QHBoxLayout>
#include <QTabWidget>

#include "core/services/ServiceLocator.h"

namespace {
     constexpr int StatusBarHeight = 50;
     constexpr int ControlWidgetHeight = 280;

     constexpr int CameraGridStretch = 8;
     constexpr int RightPanelStretch = 15;
}


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
     setupUi();
}

void MainWidget::setupUi()
{
     auto *mainLayout = new QHBoxLayout;
     auto *rightLayout = new QVBoxLayout;

     setupLeftPanel(mainLayout);
     setupRightPanel(rightLayout);

     mainLayout->addLayout(rightLayout, RightPanelStretch);
     mainLayout->setContentsMargins(0, 0, 0, 0);
     mainLayout->setSpacing(0);

     setLayout(mainLayout);
}

void MainWidget::setupLeftPanel(QHBoxLayout *mainLayout)
{
     auto *cameraGrid = new CameraGridWidget;
     mainLayout->addWidget(cameraGrid, CameraGridStretch);
}

void MainWidget::setupRightPanel(QVBoxLayout *rightLayout)
{
     auto *tabWidget = createTabWidget();

     auto* statusBarModel = new StatusBarModel(this);
     auto *statusBar = new StatusBarView;
     statusBar->setModel(statusBarModel);
     statusBar->setFixedHeight(StatusBarHeight);

     rightLayout->addWidget(tabWidget);
     rightLayout->addWidget(statusBar);

     rightLayout->setSpacing(6);
}

QTabWidget* MainWidget::createTabWidget() {
     auto *settingsWidget = new SettingsWidget;
     auto *cameraSettingsTab = new CameraSettingsTab;
     auto *controlPage = createControlPage();
     auto *logAndConfigTab = new LogAndConfigTab;
     // auto *partiesPage = createPartiesPage();

     auto *tabWidget = new QTabWidget;
     tabWidget->addTab(controlPage, tr("Graduation"));
     tabWidget->addTab(settingsWidget, tr("Settings"));
     tabWidget->addTab(cameraSettingsTab, tr("Cameras"));
     // tabWidget->addTab(partiesPage, tr("Parties"));
     tabWidget->addTab(logAndConfigTab, tr("Additional Settings"));

     connect(tabWidget, &QTabWidget::currentChanged, this, [](int index) {
          if (index == 0) {
               ServiceLocator::instance().graduationService()->prepare();
          }
     });

     return tabWidget;
}

QWidget* MainWidget::createControlPage() {
     auto *page = new QWidget;
     auto *layout = new QVBoxLayout(page);

     auto *graduationTableWidget = new GraduationTableWidget;
     auto *controlWidget = new ControlWidget;

     controlWidget->setFixedHeight(ControlWidgetHeight);

     layout->addWidget(graduationTableWidget);
     layout->addWidget(controlWidget);

     layout->setSpacing(6);

     return page;
}

