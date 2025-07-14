#include "mainwindow.h"
#include "ffmpegutils.h"
#include "ffmpegstatusdialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QLocale>
#include <QTranslator>
#include <QThread>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("QFF Media Converter");
    QCoreApplication::setOrganizationName("Bitmutex Technologies");

    FfmpegStatusDialog dlg;
    dlg.setStatus("Checking for FFmpeg...");
    dlg.show();
    qApp->processEvents();

    QString ffmpegError;
    if (!FFMpegUtils::isFfmpegAvailable(&ffmpegError)) {
        dlg.setStatus("FFmpeg not found. Downloading...");
        QString installError;
        if (!FFMpegUtils::downloadAndInstallFfmpeg(&installError)) {
            dlg.setStatus("Download failed: " + installError);
            QThread::sleep(2);
            dlg.close();
            // Handle error...
        } else {
            dlg.setStatus("FFmpeg installed successfully!");
            QThread::sleep(2);
            dlg.close();
        }
    } else {
        dlg.close();
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "qffgui_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    MainWindow window;
    //window.setWindowIcon(QIcon("C:/Users/bigwiz/Pictures/6.png"));
    window.setWindowIcon(QIcon(":/icon/6.png"));
    window.setWindowTitle("QFF Media Converter");
    window.resize(500, 800);
    window.show();

    return app.exec();
}
