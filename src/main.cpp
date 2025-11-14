#include "core/services/ServiceLocator.h"
#include "gui/windows/MainWindow.h"
#include "core/services/pressureController/impl/PressureControllerStand4.h"
#include "core/services/pressureController/impl/PressureControllerStand5.h"
#include "defines.h"
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QThread>
#include <QDebug>


void loadStyle(QApplication& app)
{
    app.setStyle("Fusion");

    QFont appfont("Roboto Mono");
    appfont.setPointSizeF(12);
    app.setFont(appfont);

    // Загрузка QSS из файла
    QFile styleFile(":/styles/styles/style.qss");  // если внутри ресурсов

    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        QString style = stream.readAll();
        app.setStyleSheet(style);
        styleFile.close();
    }
    else {
        qWarning("Failed to load style file.");
    }
}

int main(int argc, char *argv[]) {
    QString err;
    bool isOk;

    QApplication a(argc, argv);

    qRegisterMetaType<Pressure>("Pressure");

    auto* pressureSensorThread = new QThread;
    auto* pressureControllerThread = new QThread;

    loadStyle(a);

    QTranslator translator;
    if (translator.load("ru_RU", ":/translations/translations")) {
        qApp->installTranslator(&translator);
    }

    qDebug() << "Drivers:" << QSqlDatabase::drivers();

    // Инициализация сервисов
    Logger* logger = new Logger();
    logger->init("logs.txt");
    ServiceLocator::instance().setLogger(logger);

    // Загрузка конфигурации
    ConfigManager* configManager = new ConfigManager("config.json");
    ServiceLocator::instance().setConfigManager(configManager);
    if (!configManager->load()) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка конфигурации"),
            QString::fromWCharArray(L"Не удалось загрузить файл config.json.\n"),
            QString::fromWCharArray(L"Проверьте, что он находится рядом с исполняемым файлом.")
        );
        return -1;
    }

    GaugeCatalog* gaugeCatalogService = new GaugeCatalog();
    ServiceLocator::instance().setGaugeCatalog(gaugeCatalogService);
    if (!gaugeCatalogService->loadFromDirectory("gauges")) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка загрузки каталога измерительных приборов"),
            QString::fromWCharArray(L"Не удалось загрузить каталог измерительных приборов из директории /gauges/."),
            QString::fromWCharArray(L"Проверьте наличие директории /gauges/.")
        );
        return -1;
    }

    GraduationService *graduationService = new GraduationService();
    ServiceLocator::instance().setGraduationService(graduationService);

    PartyManager* partyManager = new PartyManager();
    ServiceLocator::instance().setPartyManager(partyManager);
    if (!partyManager->initDatabase("parties.sqlite")) {
        QMessageBox::critical(
            nullptr,
            QString::fromWCharArray(L"Ошибка инициализации базы данных"),
            QString::fromWCharArray(L"Не удалось инициализировать базу данных parties.sqlite."),
            QString::fromWCharArray(L"Проверьте доступность файла parties.sqlite.")
        );
        return -1;
    }

    PressureControllerBase* pressureControllerService = nullptr;
    switch (configManager->get<int>(CFG_KEY_STAND_NUMBER, 0)) {
        case 4:
            pressureControllerService = new PressureControllerStand4();
            break;
        case 5:
            pressureControllerService = new PressureControllerStand5();
            break;
    }
    if (pressureControllerService) {
        pressureControllerService->moveToThread(pressureControllerThread);
        pressureControllerThread->start();
        ServiceLocator::instance().setPressureController(pressureControllerService);
    }
    else {
        QMessageBox::critical(
                nullptr,
                QString::fromWCharArray(L"Ошибка конфигурации"),
                QString::fromWCharArray(L"Некорректный номер стенда в файле config.json."),
                QString::fromWCharArray(L"Проверьте параметр stand.number в файле config.json.")
            );
        return -1;
    }

    auto* cameraProcessorService = new CameraProcessor();
    ServiceLocator::instance().setCameraProcessor(cameraProcessorService);


    PressureSensor* pressureSensorService = new PressureSensor();
    ServiceLocator::instance().setPressureSensor(pressureSensorService);
    pressureSensorService->moveToThread(pressureSensorThread);
    pressureSensorThread->start();
    QObject::connect(pressureSensorThread, &QThread::started, [=]() {
        QString err;
        bool isOk = pressureSensorService->openCOM(configManager->get<QString>("pressureSensor.comPort", "COM1"), err);
        if (isOk) {
            QMetaObject::invokeMethod(pressureSensorService, "start", Qt::QueuedConnection);
        }
    });

//    connect(pressureSensorService, &PressureSensor::pressureMeasured, pressureControllerService, &PressureControllerBase::updatePressure);

    auto* mainWindow = new MainWindow();
    mainWindow->setMinimumSize(1600, 800);
    mainWindow->show();

    if (configManager->get<bool>(CFG_KEY_SYS_AUTOOPEN, false)) {
        cameraProcessorService->setCameraString(configManager->get<QString>(CFG_KEY_SYS_CAMERA_STR));
    }
    if (configManager->get<bool>(CFG_KEY_DRAW_CROSSHAIR, false)) {
        cameraProcessorService->enableDrawingCrosshair(true);
    }

    return QApplication::exec();
}
