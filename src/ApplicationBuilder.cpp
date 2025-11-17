#include "ApplicationBuilder.h"
#include "core/services/ServiceLocator.h"
#include "core/services/pressureController/impl/PressureControllerStand4.h"
#include "core/services/pressureController/impl/PressureControllerStand5.h"
#include "defines.h"
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>

ApplicationBuilder::ApplicationBuilder(QApplication* app)
    : app_(app)
{
}

ApplicationBuilder::~ApplicationBuilder()
{
    auto &locator = ServiceLocator::instance();

    if (auto *graduationService = locator.graduationService(); graduationService) {
        if (graduationService->isRunning()) {
            graduationService->stop();
        }
    }

    if (auto *sensor = locator.pressureSensor(); sensor) {
        QMetaObject::invokeMethod(sensor, "stop", Qt::BlockingQueuedConnection);
        sensor->deleteLater();
    }
    pressureSensorThread_.requestInterruption();
    pressureSensorThread_.quit();
    pressureSensorThread_.wait();

    if (auto *controller = locator.pressureController(); controller) {
        QMetaObject::invokeMethod(controller, "stop", Qt::BlockingQueuedConnection);
        controller->deleteLater();
    }
    pressureControllerThread_.requestInterruption();
    pressureControllerThread_.quit();
    pressureControllerThread_.wait();
}

MainWindow* ApplicationBuilder::build()
{
    loadStyle();
    loadTranslations();
    initLogger();
    loadConfig();
    initGaugeCatalog();
    initGraduation();
    initPartyManager();
    initPressureController();
    initPressureSensor();
    initCameraProcessor();

    auto* mainWindow = new MainWindow();
    mainWindow->setMinimumSize(1600, 800);

    auto* cfg = ServiceLocator::instance().configManager();
    auto* camera = ServiceLocator::instance().cameraProcessor();

    if (cfg->get<bool>(CFG_KEY_SYS_AUTOOPEN, false)) {
        camera->setCameraString(cfg->get<QString>(CFG_KEY_SYS_CAMERA_STR));
    }
    if (cfg->get<bool>(CFG_KEY_DRAW_CROSSHAIR, false)) {
        camera->enableDrawingCrosshair(true);
    }

    return mainWindow;
}

void ApplicationBuilder::declareMetatypes() {
    qRegisterMetaType<Pressure>("Pressure");
}

void ApplicationBuilder::loadStyle()
{
    app_->setStyle("Fusion");

    QFont appfont("Roboto Mono");
    appfont.setPointSizeF(12);
    app_->setFont(appfont);

    QFile styleFile(":/styles/styles/style.qss");

    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        app_->setStyleSheet(stream.readAll());
        styleFile.close();
    } else {
        qWarning("Failed to load style file.");
    }
}

void ApplicationBuilder::loadTranslations()
{
    if (translator_.load("ru_RU", ":/translations/translations")) {
        app_->installTranslator(&translator_);
    }
}

void ApplicationBuilder::initLogger()
{
    auto* logger = new Logger();
    logger->init("logs.txt");
    ServiceLocator::instance().setLogger(logger);
}

void ApplicationBuilder::loadConfig()
{
    auto* cfg = new ConfigManager("config.json");
    ServiceLocator::instance().setConfigManager(cfg);

    if (!cfg->load()) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка конфигурации"),
            QString::fromWCharArray(L"Не удалось загрузить файл config.json.\nПроверьте его наличие.")
        );
        exit(-1);
    }
}

void ApplicationBuilder::initGaugeCatalog()
{
    auto* gauge = new GaugeCatalog();
    ServiceLocator::instance().setGaugeCatalog(gauge);

    if (!gauge->loadFromDirectory("gauges")) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка загрузки каталога измерительных приборов"),
            QString::fromWCharArray(L"Не удалось загрузить каталог из /gauges/.\nПроверьте директорию.")
        );
        exit(-1);
    }
}

void ApplicationBuilder::initGraduation()
{
    auto* service = new GraduationService();
    ServiceLocator::instance().setGraduationService(service);
}

void ApplicationBuilder::initPartyManager()
{
    auto* pm = new PartyManager();
    ServiceLocator::instance().setPartyManager(pm);

    if (!pm->initDatabase("parties.sqlite")) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка инициализации базы данных"),
            QString::fromWCharArray(L"Не удалось открыть parties.sqlite.\nПроверьте файл.")
        );
        exit(-1);
    }
}

void ApplicationBuilder::initPressureController()
{
    auto* cfg = ServiceLocator::instance().configManager();

    PressureControllerBase* controller = nullptr;

    switch (cfg->get<int>(CFG_KEY_STAND_NUMBER, 0)) {
        case 4: controller = new PressureControllerStand4(); break;
        case 5: controller = new PressureControllerStand5(); break;
        default:
            QMessageBox::critical(
                nullptr,
                QString::fromWCharArray(L"Ошибка конфигурации"),
                QString::fromWCharArray(L"Некорректный номер стенда.\nПроверьте stand.number.")
            );
            exit(-1);
    }

    controller->moveToThread(&pressureControllerThread_);
    pressureControllerThread_.start();

    ServiceLocator::instance().setPressureController(controller);
}

void ApplicationBuilder::initPressureSensor()
{
    auto* cfg = ServiceLocator::instance().configManager();

    auto* sensor = new PressureSensor();
    sensor->moveToThread(&pressureSensorThread_);

    ServiceLocator::instance().setPressureSensor(sensor);

    QObject::connect(&pressureSensorThread_, &QThread::started, [=]() {
        QString err;
        bool ok = sensor->openCOM(cfg->get<QString>("pressureSensor.comPort", "COM1"), err);
        if (ok)
            QMetaObject::invokeMethod(sensor, "start", Qt::QueuedConnection);
    });

    pressureSensorThread_.start();
}

void ApplicationBuilder::initCameraProcessor()
{
    auto* cam = new CameraProcessor();
    ServiceLocator::instance().setCameraProcessor(cam);
}