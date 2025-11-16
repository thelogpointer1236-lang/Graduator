#ifndef GRADUATOR_APPLICATIONBUILDER_H
#define GRADUATOR_APPLICATIONBUILDER_H
#include "gui/windows/MainWindow.h"
#include <QTranslator>
#include <QThread>

class ApplicationBuilder final
{
public:
    ApplicationBuilder(QApplication* app);
    ~ApplicationBuilder();
    MainWindow* build();

private:
    void loadStyle();
    void loadTranslations();
    void initLogger();
    void loadConfig();
    void initGaugeCatalog();
    void initGraduation();
    void initPartyManager();
    void initPressureController();
    void initPressureSensor();
    void initCameraProcessor();

private:
    QApplication* app_;
    QTranslator translator_;

    QThread pressureSensorThread_;
    QThread pressureControllerThread_;
};


#endif //GRADUATOR_APPLICATIONBUILDER_H