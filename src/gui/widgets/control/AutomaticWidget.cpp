#include "AutomaticWidget.h"

#include "core/services/ServiceLocator.h"

#include <QComboBox>
#include <QMetaObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>

AutomaticWidget::AutomaticWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    connectSignals();
}

AutomaticWidget::~AutomaticWidget() = default;

void AutomaticWidget::setupUi()
{
    m_startButton = new QPushButton(tr("Start"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_startButton);
    mainLayout->addWidget(m_stopButton);
    mainLayout->addStretch();
    setLayout(mainLayout);
}

void AutomaticWidget::connectSignals()
{
    auto *graduationService = ServiceLocator::instance().graduationService();
    connect(m_startButton, &QPushButton::clicked,
            this, &AutomaticWidget::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked,
            this, &AutomaticWidget::onStopClicked);
    connect(graduationService, &GraduationService::started,
            this, [this]() {
                m_startButton->setEnabled(false);
                m_stopButton->setEnabled(true);
            });
    connect(graduationService, &GraduationService::interrupted,
            this, [this]() {
                m_startButton->setEnabled(true);
                m_stopButton->setEnabled(false);
            });
    connect(graduationService, &GraduationService::ended,
            this, [this]() {
                m_startButton->setEnabled(true);
                m_stopButton->setEnabled(false);
            });


}

void AutomaticWidget::onStartClicked()
{
    auto *gs = ServiceLocator::instance().graduationService();
    if (gs->state() == GraduationService::State::Running) return;

    QString err;
    gs->prepare(&err);
    if (!err.isEmpty()) {
        QMessageBox::critical(
            this,
            tr("Failed to start calibration"),
            err,
            tr("Ok")
        );
        return;
    }

    gs->start();
}

void AutomaticWidget::onStopClicked()
{
    auto *gs = ServiceLocator::instance().graduationService();
    if (gs->state() == GraduationService::State::Running) gs->interrupt();
}
