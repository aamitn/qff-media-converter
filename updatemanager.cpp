#include "updatemanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProgressDialog>
#include <QFile>

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

    QString appDir = QCoreApplication::applicationDirPath();
    QString updateFilePath = appDir + "/update.zip"; // Or use QDir::separator()

    downloadFile = new QFile(updateFilePath);
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        emit error(QString("Could not open file for download: %1").arg(downloadFile->errorString()));
        delete downloadFile;
        downloadFile = nullptr;
        return;
    }

    QNetworkRequest request(url);
    downloadReply = networkManager->get(request);


    QProgressDialog *progressDialog = new QProgressDialog(nullptr);
    progressDialog->setWindowTitle("Downloading Update...");
    progressDialog->setLabelText("Please wait while the update is being downloaded...");
    progressDialog->setRange(0, 100);
    connect(downloadReply, &QNetworkReply::downloadProgress, [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
        progressDialog->setValue(int((bytesReceived * 100.0 / bytesTotal) + 1));
    });

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

    QProgressDialog *progressDialog = qobject_cast<QProgressDialog*>(sender());
    if (progressDialog != nullptr)
        progressDialog->close();

    downloadFile->close();

    if (downloadReply->error() == QNetworkReply::NoError) {
        emit updateDownloadFinished(true);

        // Important: Pass the path to the downloaded zip and the temp dir where it's located
        initiateUpdateProcess(downloadFile->fileName(), tempUpdateDir->path());
    } else {
        emit updateDownloadFinished(false, QString("Update download failed: %1").arg(downloadReply->errorString()));
       // tempUpdateDir->remove(); // Clean up temp dir on failure
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

    // Step 2: Prepare script and launch
    QString appInstallDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    QString updaterScriptPath = appInstallDir + "/update_helper.cmd";
#elif defined(Q_OS_UNIX)
    QString updaterScriptPath = appInstallDir + "/update_helper.sh";
    QFile scriptFile(updaterScriptPath);
    if (scriptFile.exists()) {
        scriptFile.setPermissions(scriptFile.permissions() | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    }
#else
    emit error("Unsupported OS for update.");
    return;
#endif

    if (!QFile::exists(updaterScriptPath)) {
        emit error("Updater script not found at: " + updaterScriptPath);
        return;
    }

    qDebug() << "Launching updater without arguments:" << updaterScriptPath;
    QProcess::startDetached(updaterScriptPath);

    emit updateInitiated();
    QCoreApplication::quit();
}
