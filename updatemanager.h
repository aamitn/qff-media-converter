#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QFile>
#include <QCoreApplication> // For applicationVersion() and applicationDirPath()

class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager();

    // Call this method to start checking for updates
    void checkForUpdates();

signals:
    // Signals to communicate update status
    void updateCheckFinished(bool updateAvailable, const QString &latestVersion);
    void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateDownloadFinished(bool success, const QString &errorMessage = QString());
    void updateInitiated();
    void error(const QString &message); // Generic error signal

private slots:
    void handleLatestReleaseReply();
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadFinished();
    void handleReadyRead();

private:
    void downloadUpdate(const QString &url);
    void initiateUpdateProcess(const QString &zipFilePath, const QString &tempDirPath);

    QNetworkAccessManager *networkManager;
    QNetworkReply *downloadReply;
    QTemporaryDir *tempUpdateDir;
    QFile *downloadFile;

    // Update Check Configuration for your GitHub repository
    const QString GITHUB_OWNER = "aamitn"; // <<--- REPLACE THIS
    const QString GITHUB_REPO = "winhider"; // <<--- REPLACE THIS

    // Optional: Define your application's current version
    // It's often set via CMake or .pro file, and accessed via QCoreApplication::applicationVersion()
    // but you can pass it to the constructor or update it if needed.
    QString currentAppVersion;
};

#endif // UPDATEMANAGER_H
