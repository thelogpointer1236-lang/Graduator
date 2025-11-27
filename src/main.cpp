#include <QApplication>
#include "ApplicationBuilder.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ApplicationBuilder builder(&app);
    MainWindow* mainWindow = builder.build();
    mainWindow->showMaximized();

    return app.exec();
}