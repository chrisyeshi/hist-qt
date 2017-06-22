#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <opengl.h>

int main(int argc, char *argv[])
{
    // set opengl version to 3.3 core profile
    QSurfaceFormat surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setVersion(3, 3);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    surfaceFormat.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
    // set shared opengl context
    QCoreApplication::setOrganizationName("VIDi");
    QCoreApplication::setOrganizationDomain("vidi.cs.ucdavis.edu");
    QCoreApplication::setApplicationName("Histograms");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    // initiate the application
    QApplication a(argc, argv);
    // main window
    MainWindow w;
    w.show();
    return a.exec();
}
