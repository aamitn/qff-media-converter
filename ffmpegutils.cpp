#include "ffmpegutils.h"
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif


bool FFMpegUtils::isFfmpegAvailable(QString *errorMsg)
{
    // First try with current environment
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    process.waitForFinished(); // Wait for the process to finish

    int exitCode = process.exitCode();
    QString output = process.readAllStandardOutput();

    if (exitCode == 0 && output.contains("ffmpeg version")) {
        qDebug() << "FFmpeg is installed.";
        return true;
    } else {
        qDebug() << "FFmpeg is NOT installed or not found in PATH.";
        return false;
    }
    return true;
}


bool FFMpegUtils::downloadAndInstallFfmpeg(QString *errorMsg)
{
#ifdef Q_OS_WIN
    // Download static build from gyan.dev (or another trusted source)
    //QString url = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";
    QString url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip";

    QString destDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ffmpeg";
    QString zipPath = destDir + "/ffmpeg.zip";

    QDir().mkpath(destDir);

    // Download
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        if (errorMsg) *errorMsg = "Failed to download ffmpeg: " + reply->errorString();
        reply->deleteLater();
        return false;
    }

    QFile file(zipPath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMsg) *errorMsg = "Failed to save ffmpeg.zip";
        reply->deleteLater();
        return false;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();

    // Extract (requires external tool or Qt 6.5+ QZipReader, or use 7z if available)
    // Call Python unzip utility and add ffmpeg to path
    QProcess unzipProcess;
    QString pythonExe = "python"; // or "python3" if needed
    QStringList args;
    args << QDir::toNativeSeparators(zipPath)
         << QDir::toNativeSeparators(destDir);
    unzipProcess.start(pythonExe, QStringList() << "unzip_ffmpeg.py" << args);
    if (!unzipProcess.waitForStarted() || !unzipProcess.waitForFinished()) {
        if (errorMsg) *errorMsg = "Failed to run Python unzip utility.";
        return false;
    }
    if (unzipProcess.exitCode() != 0) {
        if (errorMsg) *errorMsg = "Python unzip utility failed:\n" + unzipProcess.readAllStandardError();
        return false;
    }

    if (errorMsg) *errorMsg = "FFmpeg downloaded, extracted, and PATH updated.";
    return true;
#else
    if (errorMsg) *errorMsg = "Please install ffmpeg using your package manager (e.g., sudo apt install ffmpeg).";
    return false;
#endif
}

QString FFMpegUtils::getFfmpegPath()
{
#ifdef Q_OS_WIN
    QProcess proc;
    proc.start("where", QStringList() << "ffmpeg");
    proc.waitForFinished();
    QString ffmpegPath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (ffmpegPath.isEmpty())
        ffmpegPath = "FFmpeg not found in PATH.";
    return ffmpegPath;
#else
    QProcess proc;
    proc.start("which", QStringList() << "ffmpeg");
    proc.waitForFinished();
    QString ffmpegPath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (ffmpegPath.isEmpty())
        ffmpegPath = "FFmpeg not found in PATH.";
    return ffmpegPath;
#endif
}


QString FFMpegUtils::getFfmpegVersion() {
    QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
    if (ffmpegPath.isEmpty()) {
        return "FFmpeg not found in system PATH.";
    }

    QProcess process;
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    process.start(ffmpegPath, QStringList() << "-version");
    if (!process.waitForStarted(2000) || !process.waitForFinished(2000)) {
        return "Failed to run ffmpeg.";
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    if (output.isEmpty()) {
        output = QString::fromLocal8Bit(process.readAllStandardError());
    }

    return output.isEmpty() ? "No output from ffmpeg." : output;
}
