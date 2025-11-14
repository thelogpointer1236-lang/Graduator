#include "MainWidget.h"
#include "cameraGrid/CameraGridWidget.h"
#include "control/ControlWidget.h"
#include "GraduationTableWidget.h"
#include "gui/views/StatusBarView.h"
#include "gui/models/StatusBarModel.h"
#include "SettingsWidget.h"
#include <QHBoxLayout>
#include <QTabWidget>

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

QTabWidget* MainWidget::createTabWidget()
{
     auto *settingsWidget = new SettingsWidget;
     auto *controlPage = createControlPage();

     auto *tabWidget = new QTabWidget;
     tabWidget->addTab(controlPage, tr("Graduation"));
     tabWidget->addTab(settingsWidget, tr("Settings"));

     return tabWidget;
}

QWidget* MainWidget::createControlPage()
{
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
