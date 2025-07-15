#include "mainwindow.h"
#include "ffmpegutils.h"
#include "ffmpegstatusdialog.h"
#include "pythoninstaller.h"

#include <QApplication>
#include <QMessageBox>
#include <QLocale>
#include <QTranslator>
#include <QThread>
#include <QIcon>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("QFF Media Converter");
    QCoreApplication::setOrganizationName("Bitmutex Technologies");

    PythonInstaller pythonInstaller;
    QString installerPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + PYTHON_INSTALLER_NAME;

    if (!pythonInstaller.isPythonAvailable()) {
        QMessageBox::information(nullptr, "Python Not Found", "Python not found. Downloading and installing...");

        #ifdef Q_OS_WIN
                if (!pythonInstaller.downloadInstaller(installerPath)) {
                    QMessageBox::critical(nullptr, "Download Failed", "Could not download Python installer.");
                    return -1;
                }
        #endif

        if (!pythonInstaller.installSilently(installerPath)) {
            QMessageBox::critical(nullptr, "Installation Failed", "Python installation failed.");
            return -1;
        }
        QMessageBox::information(nullptr, "Python Installed", "Python installed successfully.");
    }

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
    window.setWindowIcon(QIcon(":/icon/6.png"));
    window.setWindowTitle("QFF Media Converter");
    window.resize(500, 870);
    window.show();

    return app.exec();
}
