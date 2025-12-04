#include "ManualWidget.h"
#include "core/services/ServiceLocator.h"
#include <QGroupBox>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QGridLayout>


ManualWidget::ManualWidget(QWidget *parent)
    : QWidget(parent)
    , m_pollTimer(new QTimer(this)) // (бывший m_switchLimitTimer)
{
    setupUi();
    connectSignals(); // connectSignals() теперь только для кнопок

    // Настраиваем таймер, но НЕ запускаем его
    m_pollTimer->setInterval(100);
    connect(m_pollTimer, &QTimer::timeout, this, &ManualWidget::onPollTimerTimeout);

    // Устанавливаем корректное начальное состояние UI
    updateUiState();
}

ManualWidget::~ManualWidget() = default;

// ... (setupUi и все функции create*Group остаются БЕЗ ИЗМЕНЕНИЙ) ...

void ManualWidget::connectSignals()
{
    // Только подключения кнопок
    connect(m_btnStart, &QPushButton::clicked, this, &ManualWidget::onStartMotor);
    connect(m_btnStopMotor, &QPushButton::clicked, this, &ManualWidget::onStopMotor);
    connect(m_btnOpenInlet, &QPushButton::clicked, this, &ManualWidget::onOpenInlet);
    connect(m_btnOpenOutlet, &QPushButton::clicked, this, &ManualWidget::onOpenOutlet);
    connect(m_btnCloseBoth, &QPushButton::clicked, this, &ManualWidget::onCloseBoth);

    // Следим за изменением состояния контроллера, чтобы разблокировать
    // обратный ход, когда стартовый концевик освобожден после работы
    // автоматического режима.
    auto *pressureController = ServiceLocator::instance().pressureController();
    connect(pressureController, &PressureControllerBase::interrupted,
            this, &ManualWidget::updateUiState);
    connect(pressureController, &PressureControllerBase::successfullyStopped,
            this, &ManualWidget::updateUiState);
}

// -----------------------------------------------------------------
// НОВАЯ И ИЗМЕНЕННАЯ ЛОГИКА СЛОТОВ
// -----------------------------------------------------------------

/**
 * @brief Централизованная функция обновления состояния UI.
 * Читает состояние контроллера и приводит UI в соответствие.
 */
void ManualWidget::updateUiState()
{
    auto *pc = ServiceLocator::instance().pressureController();
    bool isRunning = pc->isRunning();
    bool isAtStartLimit = pc->isStartLimitTriggered();

    // Если UI показывает "работает", а мотор уже нет -> останавливаем таймер
    // (на случай, если он не остановился сам в onPollTimerTimeout)
    if (!isRunning && m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }

    // Обновляем кнопки мотора
    m_btnStart->setEnabled(!isRunning);
    m_btnStopMotor->setEnabled(isRunning);

    // Обновляем радио-кнопки
    m_radioForward->setEnabled(!isRunning);
    m_radioBackward->setEnabled(!isRunning);

    // Особая логика: если стоим на начальном концевике,
    // можно ехать только вперед.
    if (!isRunning && isAtStartLimit) {
        m_radioBackward->setEnabled(false);
        m_radioForward->setChecked(true);
    }
}

/**
 * @brief Слот, вызываемый таймером.
 * Активен ТОЛЬКО когда мотор запущен.
 * Проверяет условия остановки (концевик или внешняя остановка).
 */
void ManualWidget::onPollTimerTimeout()
{
    auto *pc = ServiceLocator::instance().pressureController();
    bool isRunning = pc->isRunning();
    bool isAtStartLimit = pc->isStartLimitTriggered();

    // 1. Условие авто-остановки:
    // Мотор работал, но доехал до начального концевика.
    if (isRunning && isAtStartLimit) {
        pc->interrupt();       // Останавливаем мотор
        m_pollTimer->stop();   // Останавливаем НАШ таймер
        updateUiState();       // Обновляем UI (покажет "остановлено")
        return;
    }

    // 2. Условие синхронизации:
    // Наш таймер активен, но мотор по какой-то причине
    // (другой виджет, конец цикла) уже не работает.
    if (!isRunning) {
        m_pollTimer->stop();   // Просто останавливаем НАШ таймер
        updateUiState();       // Синхронизируем UI
        return;
    }

    // 3. Если isRunning() == true и нет стоп-условий,
    // таймер просто продолжает тикать, не вызывая updateUiState
    // и не конфликтуя с другими виджетами.
}

// -----------------------------------------------------------------------------
// UI SETUP
// -----------------------------------------------------------------------------
void ManualWidget::setupUi()
{
    auto *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(createDirectionGroup(), 0, 0);
    mainLayout->addWidget(createMotorGroup(),     1, 0);
    mainLayout->addWidget(createValveGroup(),     0, 1, 2, 1);
    setLayout(mainLayout);
}

QGroupBox *ManualWidget::createDirectionGroup()
{
    auto *group  = new QGroupBox(tr("Engine direction"), this);
    auto *layout = new QHBoxLayout;

    m_radioForward  = new QRadioButton(tr("Forward"), this);
    m_radioBackward = new QRadioButton(tr("Backward"), this);
    m_radioForward->setChecked(true);

    layout->addWidget(m_radioForward);
    layout->addWidget(m_radioBackward);
    group->setLayout(layout);

    return group;
}

QGroupBox *ManualWidget::createMotorGroup()
{
    auto *group  = new QGroupBox(tr("Engine control"), this);
    auto *layout = new QVBoxLayout;

    m_btnStart     = new QPushButton(tr("Start"), this);
    m_btnStopMotor = new QPushButton(tr("Stop"),  this);
    m_btnStopMotor->setEnabled(false);

    layout->addWidget(m_btnStart);
    layout->addWidget(m_btnStopMotor);
    group->setLayout(layout);

    return group;
}

QGroupBox *ManualWidget::createValveGroup()
{
    auto *group  = new QGroupBox(tr("Valve control"), this);
    auto *layout = new QVBoxLayout;

    m_btnOpenInlet  = new QPushButton(tr("Open inlet"),  this);
    m_btnOpenOutlet = new QPushButton(tr("Open outlet"), this);
    m_btnCloseBoth  = new QPushButton(tr("Close both"),  this);

    for (auto *btn : {m_btnOpenInlet, m_btnOpenOutlet, m_btnCloseBoth}) {
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(btn);
    }

    group->setLayout(layout);
    return group;
}

/**
 * @brief Пользователь нажал "Старт".
 */
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
        return;
    }

    // Отправляем команду
    if (m_radioForward->isChecked())
        QMetaObject::invokeMethod(pressureController, "startGoToEnd", Qt::QueuedConnection);
    else
        QMetaObject::invokeMethod(pressureController, "startGoToStart", Qt::QueuedConnection);

    // --- ЛОГИКА ЗАПУСКА ---
    // 1. Запускаем НАШ таймер, который будет следить за концевиками
    m_pollTimer->start();

    // 2. Немедленно обновляем UI для отзывчивости
    // (Не ждем, пока QueuedConnection отработает)
    m_btnStart->setEnabled(false);
    m_btnStopMotor->setEnabled(true);
    m_radioForward->setEnabled(false);
    m_radioBackward->setEnabled(false);
}

/**
 * @brief Пользователь нажал "Стоп".
 */
void ManualWidget::onStopMotor()
{
    // --- ЛОГИКА ОСТАНОВКИ ---
    // 1. Немедленно останавливаем НАШ таймер
    m_pollTimer->stop();

    // 2. Отправляем команду контроллеру
    auto *pressureController = ServiceLocator::instance().pressureController();
    if (pressureController->isRunning()) {
        pressureController->interrupt();
    }

    // 3. Немедленно обновляем UI
    // (Таймер остановлен, поэтому мы можем безопасно вызвать updateUiState)
    updateUiState();
}

// ... (onOpenInlet, onOpenOutlet, onCloseBoth остаются БЕЗ ИЗМЕНЕНИЙ) ...

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