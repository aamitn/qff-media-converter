#include "pythoninstaller.h"
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>

PythonInstaller::PythonInstaller(QObject *parent)
    : QObject(parent), detectedExecutable("python3")
{}

bool PythonInstaller::isPythonAvailable() {
    if (QProcess::execute("python3", {"--version"}) == 0) {
        detectedExecutable = "python3";
        return true;
    } else if (QProcess::execute("python", {"--version"}) == 0) {
        detectedExecutable = "python";
        return true;
    }
    return false;
}

bool PythonInstaller::downloadInstaller(const QString &savePath) {
#ifdef Q_OS_WIN
    QNetworkAccessManager manager;
    QUrl url(PYTHON_DOWNLOAD_URL);

    qDebug() << "Downloading Python from:" << url.toString();
    qDebug() << "SSL support:" << QSslSocket::supportsSsl();

    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Download failed:" << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << savePath;
        reply->deleteLater();
        return false;
    }

    file.write(reply->readAll());
    file.close();
    reply->deleteLater();
    qDebug() << "Download successful.";
    return true;
#else
    Q_UNUSED(savePath);
    return true; // Not needed on Linux/macOS
#endif
}

bool PythonInstaller::installSilently(const QString &installerPath) {
#ifdef Q_OS_WIN
    QStringList args = { "/quiet", "InstallAllUsers=1", "PrependPath=1", "Include_test=0" };
    int result = QProcess::execute(installerPath, args);
    return result == 0;

#elif defined(Q_OS_LINUX)
    QStringList packageManagers = { "apt", "dnf", "pacman" };
    QStringList installCommands = {
        "sudo apt update && sudo apt install -y python3",
        "sudo dnf install -y python3",
        "sudo pacman -Sy --noconfirm python"
    };

    for (int i = 0; i < packageManagers.size(); ++i) {
        if (QProcess::execute("which", { packageManagers[i] }) == 0) {
            return QProcess::execute("/bin/bash", { "-c", installCommands[i] }) == 0;
        }
    }

    qDebug() << "No supported package manager found.";
    return false;

#elif defined(Q_OS_MACOS)
    if (QProcess::execute("which", { "brew" }) == 0) {
        return QProcess::execute("brew", { "install", "python3" }) == 0;
    } else {
        qDebug() << "Homebrew not found. Please install it manually.";
        return false;
    }

#else
    return false;
#endif
}

QString PythonInstaller::pythonExecutable() const {
    return detectedExecutable;
}
