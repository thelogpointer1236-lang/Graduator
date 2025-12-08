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
    auto *pressureController = ServiceLocator::instance().pressureController();
    m_calibrationModeComboBox = new QComboBox(this);
    m_calibrationModeComboBox->addItems(pressureController->getModes());
    m_startButton = new QPushButton(tr("Start"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_calibrationModeComboBox);
    mainLayout->addWidget(m_startButton);
    mainLayout->addWidget(m_stopButton);
    mainLayout->addStretch();
    setLayout(mainLayout);
}

void AutomaticWidget::connectSignals()
{
    auto *graduationService = ServiceLocator::instance().graduationService();
    connect(m_calibrationModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutomaticWidget::onCalibrationModeChanged);
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

void AutomaticWidget::onCalibrationModeChanged(int index)
{
    auto *pc = ServiceLocator::instance().pressureController();
    QMetaObject::invokeMethod(pc, "setMode", Qt::QueuedConnection, Q_ARG(int, index));
}

void AutomaticWidget::onStartClicked()
{
    auto *gs = ServiceLocator::instance().graduationService();
    if (gs->state() == GraduationService::State::Running) return;

    QString err;
    gs->prepare(err);
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
