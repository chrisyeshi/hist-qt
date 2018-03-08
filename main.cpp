#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QStyleFactory>
#include <QDebug>
#include <QMutex>

QMutex mutex;
QFile logFile("log.txt");

void messageHandler(
        QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&mutex);
    auto timeStr =
            QDateTime::currentDateTime().toString("dd-MM-yyyy HH:mm:ss:zzz");
    if (QtDebugMsg == type) {
        QTextStream(stdout) << msg << endl;
    }
    QTextStream(&logFile) << timeStr << " " << msg << endl;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(volren);
    // open the log file
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    qInstallMessageHandler(messageHandler);
    // set opengl version to 3.3 core profile
    QSurfaceFormat surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setVersion(3, 3);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    surfaceFormat.setOption(QSurfaceFormat::DebugContext);
    surfaceFormat.setSamples(4);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
    // set shared opengl context
    QCoreApplication::setOrganizationName("VIDi");
    QCoreApplication::setOrganizationDomain("vidi.cs.ucdavis.edu");
    QCoreApplication::setApplicationName("Histograms");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    // initiate the application
    QApplication a(argc, argv);
    // main window
    MainWindow w("user study");
//    MainWindow w("physical");
    w.show();
    return a.exec();
}
