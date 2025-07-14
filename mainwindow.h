#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "audiowaveformwidget.h"
#include "updatemanager.h"

#include <QMainWindow>
#include <QProcess>
#include <QLabel>
#include <QMediaPlayer>       // For audio and video playback control
#include <QVideoWidget> // For video display
#include <QSettings>

QString formatFileSize(quint64 bytes);

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void browseFile();
    void setAutoplayEnabled(bool enabled);
    void setUpdateEnabled(bool enabled);
    bool isAutoplayEnabled() const;
    bool isUpdateEnabled() const;

private slots:
    void browseOutputFile();
    void startConversion();
    void updateLog();
    void conversionFinished(int, QProcess::ExitStatus);
    void probeFileType();
    void handleProbeFinished();
    void updateFormatCombo(const QString &type);
    void showRelevantParams(const QString &type);
    void showFPSDialog();
    void getDuration(const QString &filePath);

    // New slots for media playback
    void playMedia();
    void pauseMedia();
    void stopMedia();
    void mediaStateChanged(QMediaPlayer::PlaybackState state);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void setVolume(int volume);
    void setPosition(int position);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleFfmpegWaveformReadyRead();
    void handleFfmpegWaveformFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void handleUpdateCheckFinished(bool updateAvailable, const QString &latestVersion);
    void handleUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleUpdateDownloadFinished(bool success, const QString &errorMessage);
    void handleUpdateInitiated();
    void handleUpdateManagerError(const QString &message);


private:
    Ui::MainWindow *ui;
    QProcess *process;
    QProcess *probeProcess;
    int fpsToUse;
    QLabel *durationLabel;
    QLabel *sizeLabel;

    // New members for media playback
    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput; // For audio playback if no video
    QLabel *imagePreviewLabel; // For displaying images

    // You might also need QWidget pointers to group your media controls
   /* QWidget *videoControlsWidget;
    QWidget *audioControlsWidget;
    QWidget *imagePreviewWidget;*/

    AudioWaveformWidget *waveformWidget;

    QProcess *ffmpegWaveformProcess;
    QByteArray m_rawAudioData; // To accumulate raw audio data from FFmpeg
    qint64 m_currentMediaDuration; // Store the duration for waveform scaling

    bool m_autoplayEnabled;
    bool m_updateEnabled;

    static const QStringList SUPPORTED_VIDEO_FORMATS;
    static const QStringList SUPPORTED_AUDIO_FORMATS;
    static const QStringList SUPPORTED_IMAGE_FORMATS;
    static const QStringList SUPPORTED_GIF_FORMATS;

    QString formatFileSize(quint64 bytes);
    QString formatTime(qint64 ms); // Helper for time formatting

    UpdateManager *updateManager;
};

#endif // MAINWINDOW_H
