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
#include <QTimer>
#include <QStandardPaths>


void UpdateManager::showError(const QString &message, QWidget *parent) {
    qWarning() << "Error:" << message;
    QMessageBox::critical(parent, "Error", message);
}


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
        qWarning() << "QCoreApplication::applicationVersion() is empty. Please set it via CMake or .pro file.";
        currentAppVersion = "unknown";
    }

    // Show in debug output
    qDebug() << "App Version:" << currentAppVersion;

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
        QString versionNumber = latestVersionTag;
        if (versionNumber.startsWith('v'))
            versionNumber = versionNumber.mid(1); // remove 'v' prefix
        QString downloadUrl;

        #ifdef Q_OS_WIN
                expectedZipName = QString("QFFMediaConverter-%1-win64.exe").arg(versionNumber);
        #elif defined(Q_OS_MAC)
                expectedZipName = QString("QFFMediaConverter-%1-mac.dmg").arg(versionNumber);
        #elif defined(Q_OS_LINUX)
                expectedZipName = QString("QFFMediaConverter-%1-linux.tar.gz").arg(versionNumber);
        #else
                expectedZipName = "dist_unknown.zip";
        #endif
        // Set Assets

        QJsonArray assets = release["assets"].toArray();
        for (const QJsonValue &assetValue : assets) {
            QJsonObject asset = assetValue.toObject();
            QString assetName = asset["name"].toString();
            if (assetName.compare(expectedZipName, Qt::CaseInsensitive) == 0) {
                downloadUrl = asset["browser_download_url"].toString();

                if (downloadUrl.isEmpty()) {
                    QStringList availableAssets;
                    for (const QJsonValue &assetValue : assets) {
                        availableAssets << assetValue.toObject()["name"].toString();
                    }
                    qWarning() << "⚠️ No matching asset found for platform. Available assets:" << availableAssets;
                }

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
        QString err = QString("Failed to check for updates: %1").arg(reply->errorString());
        emit error(err);
        showError(err);
    }
    reply->deleteLater();
}

void UpdateManager::downloadUpdate(const QString &url) {
    qDebug() << "Downloading update from:" << url;

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QUrl qurl(url);
    QString fileName = QFileInfo(qurl.path()).fileName(); // e.g., "QFFMediaConverter-1.2.11-win64.exe"
    QString updateFilePath = tempDir + QDir::separator() + fileName;

    downloadFile = new QFile(updateFilePath);
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        QString err = QString("Could not open file for download: %1").arg(downloadFile->errorString());
        emit error(err);
        showError(err);

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
        initiateUpdateProcess();
    } else {
        QString err = QString("Update download failed: %1").arg(downloadReply->errorString());
        emit updateDownloadFinished(false, err);
        showError(err);
       // tempUpdateDir->remove(); // Clean up temp dir on failure
    }

    downloadReply->deleteLater();
    downloadReply = nullptr;
    delete downloadFile;
    downloadFile = nullptr;
    // tempUpdateDir will be deleted when UpdateManager is destructed
    // or you can explicitly delete it after update process is initiated if it's no longer needed for cleanup by script
}


void UpdateManager::initiateUpdateProcess() {
    qDebug() << "Initiating update process...";

    // Step 1: Get temp directory path
    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir tempDir(tempDirPath);

    // Step 2: Find installer by expected name pattern
    QStringList filter;
    #ifdef Q_OS_WIN
        filter << "QFFMediaConverter-*-win64.exe";
    #elif defined(Q_OS_MAC)
        filter << "QFFMediaConverter-*-mac.dmg";
    #elif defined(Q_OS_LINUX)
        filter << "QFFMediaConverter-*-linux.tar.gz"; // or .AppImage
    #endif

    QStringList matches = tempDir.entryList(filter, QDir::Files);
    if (matches.isEmpty()) {
        QString err = "No installer found in temp directory: " + tempDirPath;
        emit error(err);
        showError(err);
        return;
    }

    QString installerPath = tempDir.absoluteFilePath(matches.first());
    qDebug() << "Found installer:" << installerPath;

    // Step 3: Launch installer
    bool started = QProcess::startDetached(installerPath);
    if (!started) {
        QString err = "Failed to launch installer: " + installerPath;
        emit error(err);
        showError(err);
        return;
    }

    emit updateInitiated();

    // Step 4: Exit current application
    qApp->exit(0);
}
