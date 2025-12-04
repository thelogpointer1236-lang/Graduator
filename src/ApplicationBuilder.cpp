#include "ApplicationBuilder.h"
#include "core/services/ServiceLocator.h"
#include "core/services/UserDialogService.h"
#include "core/services/pressureController/impl/PressureControllerStand4.h"
#include "core/services/pressureController/impl/PressureControllerStand5.h"
#include "defines.h"
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>

#include "core/services/cameraProcessor/dshow/FrameGrabberCB.h"

ApplicationBuilder::ApplicationBuilder(QApplication* app)
    : app_(app)
{
    declareMetatypes();
}

ApplicationBuilder::~ApplicationBuilder()
{
    auto &locator = ServiceLocator::instance();

    if (auto *gs = locator.graduationService(); gs) {
        if (gs->isRunning()) {
            gs->interrupt();
        }
    }

    if (auto *sensor = locator.pressureSensor(); sensor) {
        sensor->stop();
        sensor->deleteLater();
    }
    pressureSensorThread_.requestInterruption();
    pressureSensorThread_.quit();
    pressureSensorThread_.wait();

    if (auto *controller = locator.pressureController(); controller) {
        controller->interrupt();
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
    initUserDialogService();
    initPressureController();
    initPressureSensor();
    initCameraProcessor();
    initTelemetryLogger();

    auto* mainWindow = new MainWindow();
    mainWindow->setMinimumSize(1600, 800);

    applySettings();

    return mainWindow;
}

void ApplicationBuilder::declareMetatypes() {
    qRegisterMetaType<Pressure>("Pressure");
    qRegisterMetaType<LogLevel>("LogLevel");
    qRegisterMetaType<quint8*>("quint8*");
    qRegisterMetaType<int*>("int*");
    qRegisterMetaType<QString*>("QString*");
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
    auto* cfg = ServiceLocator::instance().configManager();
    auto* pm = new PartyManager(cfg->get<int>(CFG_KEY_STAND_NUMBER));
    ServiceLocator::instance().setPartyManager(pm);
}

void ApplicationBuilder::initUserDialogService() {
    auto *service = new UserDialogService();
    ServiceLocator::instance().setUserDialogService(service);
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
        bool ok = sensor->openCOM(cfg->get<QString>("pressureSensor.comPort", "COM155"), err);
        if (ok)
            QMetaObject::invokeMethod(sensor, "start", Qt::QueuedConnection);
    });

    pressureSensorThread_.start();
}

void ApplicationBuilder::initCameraProcessor()
{
    auto* cam = new CameraProcessor(4, 640, 480);
    ServiceLocator::instance().setCameraProcessor(cam);
}

void ApplicationBuilder::initTelemetryLogger() {
    auto *cfg = ServiceLocator::instance().configManager();
    auto *logger = new TelemetryLogger(cfg->get<QString>(CFG_KEY_TELEMETRY_LOG_FOLDER, "telemetry"));
    ServiceLocator::instance().setTelemetryLogger(logger);
    QObject::connect(
        ServiceLocator::instance().pressureController()->g540Driver(), &G540Driver::started,
        logger, &TelemetryLogger::begin, Qt::QueuedConnection);
    QObject::connect(
        ServiceLocator::instance().pressureController()->g540Driver(), &G540Driver::stopped,
        logger, &TelemetryLogger::end, Qt::QueuedConnection);
}

void ApplicationBuilder::applySettings() {
    auto* cfg = ServiceLocator::instance().configManager();
    auto* camera = ServiceLocator::instance().cameraProcessor();

    if (cfg->get<bool>(CFG_KEY_SYS_AUTOOPEN, false)) {
        camera->setCameraString(cfg->get<QString>(CFG_KEY_SYS_CAMERA_STR));
    }
    if (cfg->get<bool>(CFG_KEY_AIM_VISIBLE, false)) {
        camera->setAimEnabled(true);
    }
    camera->setAimColor(cfg->get<QColor>(CFG_KEY_AIM_COLOR, QColor("#FF0000")));
    camera->setAimRadius(cfg->get<int>(CFG_KEY_AIM_RADIUS, 30));
    CameraProcessor::restoreDefaultCapRate(); // Раз в 4 кадра
}
