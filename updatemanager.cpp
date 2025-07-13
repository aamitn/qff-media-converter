#include "updatemanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent),
    networkManager(new QNetworkAccessManager(this)),
    downloadReply(nullptr),
    tempUpdateDir(nullptr),
    downloadFile(nullptr)
{
    QCoreApplication::setApplicationVersion(APP_VERSION);
    currentAppVersion = QCoreApplication::applicationVersion();
    if (currentAppVersion.isEmpty()) {
        qWarning() << "⚠️  QCoreApplication::applicationVersion() is empty. Please set it via CMake or .pro file.";
        currentAppVersion = "unknown";
    }

    // Show in debug output
    qDebug() << "✅ App Version:" << currentAppVersion;

    // Show in a message box (if GUI app)
    // QMessageBox::information(nullptr, "App Version", "Current Version: " + currentAppVersion);
}

UpdateManager::~UpdateManager() {
    // QNetworkAccessManager is parented to this, so it will be deleted automatically.
    // Ensure any open replies are deleted.
    if (downloadReply && downloadReply->isRunning()) {
        downloadReply->abort();
        downloadReply->deleteLater();
    }
    if (downloadFile && downloadFile->isOpen()) {
        downloadFile->close();
    }
    // QTemporaryDir deletes its contents on destruction, but it's good to ensure.
    if (tempUpdateDir) {
        // tempUpdateDir->remove(); // Will be removed on destruction if valid
        delete tempUpdateDir;
    }
}

void UpdateManager::checkForUpdates() {
    qDebug() << "Checking for updates...";
    QString apiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(GITHUB_OWNER).arg(GITHUB_REPO);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("%1-Updater").arg(QCoreApplication::applicationName()));

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &UpdateManager::handleLatestReleaseReply);
}

void UpdateManager::handleLatestReleaseReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject release = jsonDoc.object();

        QString latestVersionTag = release["tag_name"].toString(); // e.g., "v1.2.8"
        QString downloadUrl;

        // Find the appropriate asset (e.g., a .zip file of your app)
        QJsonArray assets = release["assets"].toArray();
        for (const QJsonValue &assetValue : assets) {
            QJsonObject asset = assetValue.toObject();
            if (asset["name"].toString().endsWith(".zip", Qt::CaseInsensitive)) { // Adjust based on your asset name
                downloadUrl = asset["browser_download_url"].toString();
                break;
            }
        }

        // Clean up version strings for comparison (remove 'v' prefix if present)
        QString cleanLatestVersion = latestVersionTag;
        if (cleanLatestVersion.startsWith("v")) {
            cleanLatestVersion.remove(0, 1);
        }
        QString cleanCurrentVersion = currentAppVersion;
        if (cleanCurrentVersion.startsWith("v")) {
            cleanCurrentVersion.remove(0, 1);
        }

        qDebug() << "Current version:" << cleanCurrentVersion << "Latest version:" << cleanLatestVersion;

        // Simple version comparison (for more robust, use QVersionNumber if Qt 5.15+ / Qt 6)
        if (cleanLatestVersion.compare(cleanCurrentVersion, Qt::CaseInsensitive) > 0 && !downloadUrl.isEmpty()) {
            emit updateCheckFinished(true, latestVersionTag); // Update available
            downloadUpdate(downloadUrl); // Automatically start download
        } else {
            emit updateCheckFinished(false, currentAppVersion); // No update
        }
    } else {
        emit error(QString("Failed to check for updates: %1").arg(reply->errorString()));
    }
    reply->deleteLater();
}

void UpdateManager::downloadUpdate(const QString &url) {
    qDebug() << "Downloading update from:" << url;

    tempUpdateDir = new QTemporaryDir();
    if (!tempUpdateDir->isValid()) {
        emit error("Could not create temporary directory for update.");
        delete tempUpdateDir;
        tempUpdateDir = nullptr;
        return;
    }

    QString localFileName = tempUpdateDir->path() + "/update.zip"; // Or a more specific name
    downloadFile = new QFile(localFileName);
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        emit error(QString("Could not open file for download: %1").arg(downloadFile->errorString()));
        tempUpdateDir->remove();
        delete tempUpdateDir;
        tempUpdateDir = nullptr;
        delete downloadFile;
        downloadFile = nullptr;
        return;
    }

    QNetworkRequest request(url);
    downloadReply = networkManager->get(request);

    connect(downloadReply, &QNetworkReply::downloadProgress, this, &UpdateManager::handleDownloadProgress);
    connect(downloadReply, &QNetworkReply::readyRead, this, &UpdateManager::handleReadyRead);
    connect(downloadReply, &QNetworkReply::finished, this, &UpdateManager::handleDownloadFinished);
}

void UpdateManager::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit updateDownloadProgress(bytesReceived, bytesTotal);
}

void UpdateManager::handleReadyRead() {
    if (downloadReply && downloadFile && downloadFile->isOpen()) {
        downloadFile->write(downloadReply->readAll());
    }
}

void UpdateManager::handleDownloadFinished() {
    if (!downloadReply) return;

    downloadFile->close();

    if (downloadReply->error() == QNetworkReply::NoError) {
        emit updateDownloadFinished(true);
        // Important: Pass the path to the downloaded zip and the temp dir where it's located
        initiateUpdateProcess(downloadFile->fileName(), tempUpdateDir->path());
    } else {
        emit updateDownloadFinished(false, QString("Update download failed: %1").arg(downloadReply->errorString()));
        tempUpdateDir->remove(); // Clean up temp dir on failure
    }

    downloadReply->deleteLater();
    downloadReply = nullptr;
    delete downloadFile;
    downloadFile = nullptr;
    // tempUpdateDir will be deleted when UpdateManager is destructed
    // or you can explicitly delete it after update process is initiated if it's no longer needed for cleanup by script
}

void UpdateManager::initiateUpdateProcess(const QString &zipFilePath, const QString &tempDirPath) {
    qDebug() << "Initiating update process...";

    QString appInstallDir = QCoreApplication::applicationDirPath();
    // Path to the update helper script you bundled with your application
    // Ensure this script (update_helper.bat/sh) is deployed with your app
#ifdef Q_OS_WIN
    QString updaterScriptPath = appInstallDir + "/update_helper.cmd";
#elif defined(Q_OS_UNIX) // Linux and macOS
    QString updaterScriptPath = appInstallDir + "/update_helper.sh";
    QFile scriptFile(updaterScriptPath);
    if (scriptFile.exists()) {
        // Ensure the script is executable on Unix-like systems
        scriptFile.setPermissions(scriptFile.permissions() | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    }
#else
    emit error("Unsupported operating system for auto-update.");
    return;
#endif

    if (!QFile::exists(updaterScriptPath)) {
        emit error(QString("Updater script not found at: %1. Please ensure it's deployed with your app.").arg(updaterScriptPath));
        tempUpdateDir->remove();
        return;
    }

    // Pass necessary arguments to the updater script
    QStringList arguments;
    arguments << QDir::toNativeSeparators(appInstallDir); // Arg 1: Target installation directory
    arguments << QDir::toNativeSeparators(tempDirPath);   // Arg 2: Temp directory where downloaded zip and maybe extracted files are
    arguments << QDir::toNativeSeparators(zipFilePath);   // Arg 3: Path to the downloaded zip file
    arguments << QCoreApplication::applicationName();     // Arg 4: Application Name (e.g., qffgui)
    arguments << QCoreApplication::applicationName() + (
#ifdef Q_OS_WIN
                     ".exe"
#else
                     "" // No extension on Linux/macOS typically
#endif
                     ); // Arg 5: Full executable name (e.g., qffgui.exe)

    qDebug() << "Launching updater:" << updaterScriptPath << arguments;
    QProcess::startDetached(updaterScriptPath, arguments);

    // After launching the updater, the main application *must* quit
    // to allow the updater to replace files.
    emit updateInitiated(); // Notify that update is starting and app will exit
    QCoreApplication::quit();

    // The tempUpdateDir will be cleaned up by the updater script or on UpdateManager destruction.
}
