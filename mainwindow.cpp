#include "mainwindow.h"
#include "ui_MainWindow.h"
#include "menubarhelper.h"
#include "audiowaveformwidget.h"

#include <QFileDialog>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QMediaMetaData>
#include <QPixmap>
#include <QAudioOutput>

// --- DEFINE THE STATIC CONST LISTS HERE (outside any function) ---
const QStringList MainWindow::SUPPORTED_VIDEO_FORMATS = {
    "mp4", "mkv", "avi", "mov", "webm", "flv", "wmv", "mpg", "3gp", "ts", "ogv", "vob", "m4v"
};
const QStringList MainWindow::SUPPORTED_AUDIO_FORMATS = {
    "mp3", "aac", "wav", "flac", "ogg", "opus", "m4a", "wma", "aiff", "ac3", "dts"
};
const QStringList MainWindow::SUPPORTED_IMAGE_FORMATS = {
    "png", "jpg", "jpeg", "bmp", "tiff", "webp", "ico", "tga", "ppm"
};
const QStringList MainWindow::SUPPORTED_GIF_FORMATS = {
    "gif"
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), process(nullptr), probeProcess(nullptr),
    mediaPlayer(nullptr), audioOutput(nullptr), videoWidget(nullptr), imagePreviewLabel(nullptr),
    waveformWidget(nullptr), ffmpegWaveformProcess(nullptr),
    m_currentMediaDuration(0),
    m_autoplayEnabled(true)
{
    ui->setupUi(this);

    // Initialize UpdateManager
    updateManager = new UpdateManager(this); // Make MainWindow the parent

    // Connect UpdateManager signals to MainWindow slots
    connect(updateManager, &UpdateManager::updateCheckFinished, this, &MainWindow::handleUpdateCheckFinished);
    connect(updateManager, &UpdateManager::updateDownloadProgress, this, &MainWindow::handleUpdateDownloadProgress);
    connect(updateManager, &UpdateManager::updateDownloadFinished, this, &MainWindow::handleUpdateDownloadFinished);
    connect(updateManager, &UpdateManager::updateInitiated, this, &MainWindow::handleUpdateInitiated);
    connect(updateManager, &UpdateManager::error, this, &MainWindow::handleUpdateManagerError);

    updateManager->checkForUpdates();

    // ---Instantiate Menu Bar ---
    setMenuBar(MenuBarHelper::createMenuBar(this));

    connect(ui->browseButton, &QPushButton::clicked, this, &MainWindow::browseFile);
    connect(ui->convertButton, &QPushButton::clicked, this, &MainWindow::startConversion);
    connect(ui->outputBrowseButton, &QPushButton::clicked, this, &MainWindow::browseOutputFile);

    ui->formatCombo->clear();


    ui->resolutionCombo->addItems({
        // Standard Definition (SD)
        "320x240",   // QVGA (4:3)
        "426x240",   // 240p (16:9)
        "640x360",   // 360p (16:9)
        "640x480",   // VGA (4:3)
        "720x480",   // NTSC/DVD (4:3 or 16:9 anamorphic)
        "854x480",   // 480p (16:9)

        // High Definition (HD)
        "1280x720",  // 720p (HD Ready)
        "1920x1080", // 1080p (Full HD)

        // Ultra High Definition (UHD) / 4K
        "2048x1080", // 2K DCI (Digital Cinema Initiatives)
        "2560x1080", // UW-UXGA (Ultrawide FHD)
        "2560x1440", // 1440p (QHD/2K)
        "3840x1600", // UW-QHD (Ultrawide 4K)
        "3840x2160", // 2160p (4K UHD)
        "4096x2160", // 4K DCI

        // Higher Resolutions (Less common for general video conversion, but included for completeness)
        "5120x2160", // UW-5K (Ultrawide 5K)
        "5120x2880", // 5K UHD+
        "7680x4320"  // 4320p (8K UHD)
    });

    ui->videoBitrateSpin->setValue(2000);
    ui->audioBitrateSpin->setValue(128); // Default to 128 kbps
    ui->sampleRateCombo->setCurrentText("44100"); // Default to 44100 Hz

    // --- Initialize Media Player and related widgets ---
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this); // For audio output
    mediaPlayer->setAudioOutput(audioOutput);


    // Reference the QVideoWidget and QLabel from the UI
    videoWidget = ui->videoPlayerWidget;
    mediaPlayer->setVideoOutput(videoWidget);

    imagePreviewLabel = new QLabel("No Image Selected", this); // You'll need a QLabel for image preview in your UI
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->imagePreviewLabel->setLayout(new QVBoxLayout());
    ui->imagePreviewLabel->layout()->addWidget(imagePreviewLabel);

    // CRITICAL: Initialize and add your AudioWaveformWidget
    waveformWidget = new AudioWaveformWidget(ui->audioWaveformPlaceholder); // Parent to the UI placeholder
    // Set a layout for the placeholder if not already set in UI designer
    if (!ui->audioWaveformPlaceholder->layout()) {
        ui->audioWaveformPlaceholder->setLayout(new QVBoxLayout());
    }
    ui->audioWaveformPlaceholder->layout()->addWidget(waveformWidget);
    waveformWidget->setVisible(false); // Hide initially

    // --- Initialize FFmpeg waveform process and connect signals ---
    ffmpegWaveformProcess = new QProcess(this);
    connect(ffmpegWaveformProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleFfmpegWaveformReadyRead);
    connect(ffmpegWaveformProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleFfmpegWaveformFinished);
    connect(ffmpegWaveformProcess, &QProcess::readyReadStandardError, [this](){
        qDebug() << "FFmpeg Waveform Error:" << ffmpegWaveformProcess->readAllStandardError();
    });


    // Connect media player signals to slots
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::mediaStateChanged);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::durationChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::positionChanged);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::handleMediaStatusChanged);

    // Connect UI buttons/sliders to media player slots
    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::playMedia);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::pauseMedia);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::stopMedia);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
    connect(ui->progressBarSlider, &QSlider::sliderMoved, this, &MainWindow::setPosition);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, waveformWidget, &AudioWaveformWidget::setCurrentPosition);

    // Initial state of media controls
    ui->playButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->volumeSlider->setValue(audioOutput->volume() * 100); // Set initial volume
    ui->progressBarSlider->setEnabled(false);
    ui->currentTimeLabel->setText("00:00");
    ui->totalTimeLabel->setText("00:00");

    // CRITICAL: Set the initial stacked widget page (image preview by default as per UI)
    ui->previewStackedWidget->setCurrentIndex(1); // Page 1 is imagePreviewPage

    // Hide all preview widgets initially
    ui->mediaControlsWidget->setVisible(false); // Assuming you'll have specific controls for audio
}



MainWindow::~MainWindow() {
    // Cleanup media player and widgets (they are parented, so mostly handled, but good practice)
    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }
    delete mediaPlayer;
    //delete audioOutput; // Only delete if not parented, if parented it's handled by Qt
    // videoWidget and imagePreviewLabel are parented to ui->videoPlayerWidget and ui->imagePreviewWidget,
    // which are deleted by Qt's parent-child hierarchy when ui is deleted.
    delete ffmpegWaveformProcess;
    delete ui;
}

void MainWindow::browseFile() {

    // --- Use the centralized lists to build the QFileDialog filter ---
    QString filter = "QFF Media Files (";

    // Helper to join formats with a wildcard prefix and space separator
    auto joinFormats = [](const QStringList& formats) {
        QStringList wildcards;
        for (const QString& format : formats) {
            wildcards << "*." + format;
        }
        return wildcards.join(" ");
    };

    filter += joinFormats(SUPPORTED_VIDEO_FORMATS) + " ";
    filter += joinFormats(SUPPORTED_AUDIO_FORMATS) + " ";
    filter += joinFormats(SUPPORTED_IMAGE_FORMATS) + " ";
    filter += joinFormats(SUPPORTED_GIF_FORMATS);
    filter += ");;All Files (*.*)"; // Option to show all files

    QString file = QFileDialog::getOpenFileName(this, "Select Input File", "", filter);

    if (!file.isEmpty()) {
        ui->inputEdit->setText(file);

        // Set default output file path
        QString ext = ui->formatCombo->currentText();
        if (ext.isEmpty()) ext = "mp4";
        QString outputFile = QFileInfo(file).absolutePath() + "/" +
                             QFileInfo(file).completeBaseName() + "_converted." + ext;
        ui->outputEdit->setText(outputFile);

        probeFileType();
        getDuration(file);

        // --- Prepare for media preview ---
        // Stop any currently playing media
        if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
            mediaPlayer->stop();
        }
        mediaPlayer->setSource(QUrl::fromLocalFile(file));

        // Reset progress bar
        ui->progressBarSlider->setValue(0);
        ui->currentTimeLabel->setText("00:00");
        ui->totalTimeLabel->setText("00:00");
    }
}

void MainWindow::browseOutputFile() {
    QString file = QFileDialog::getSaveFileName(this, "Select Output File", ui->outputEdit->text());
    if (!file.isEmpty()) {
        ui->outputEdit->setText(file);
    }
}

void MainWindow::probeFileType() {
    QString inputFile = ui->inputEdit->text();
    if (inputFile.isEmpty()) return;

    if (probeProcess) {
        probeProcess->deleteLater();
    }
    probeProcess = new QProcess(this);
    connect(probeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleProbeFinished);

    // Use JSON output for better parsing
    QStringList args = {
        "-v", "quiet",
        "-print_format", "json",
        "-show_streams",
        "-show_format",
        inputFile
    };
    probeProcess->start("ffprobe", args);

}

void MainWindow::handleProbeFinished() {
    QByteArray output = probeProcess->readAllStandardOutput();
    QByteArray errorOutput = probeProcess->readAllStandardError();

    ui->logOutput->append("ffprobe stdout: " + QString::fromLocal8Bit(output));
    ui->logOutput->append("ffprobe stderr: " + QString::fromLocal8Bit(errorOutput));

    QString detectedType = "unknown";
    bool isGif = false;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &jsonError);

    qint64 mediaDuration = 0; // Initialize to 0


    if (jsonError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        QJsonArray streams = obj.value("streams").toArray();
        QJsonObject format = obj.value("format").toObject();
        QString formatName = format.value("format_name").toString();
        // Extract duration from format object if available
        if (format.contains("duration")) {
            mediaDuration = qRound(format.value("duration").toString().toDouble() * 1000); // Convert to ms
        }


        bool hasVideo = false, hasAudio = false, isImage = false;
        for (const QJsonValue &streamVal : streams) {
            QJsonObject stream = streamVal.toObject();
            QString codecType = stream.value("codec_type").toString();
            QString codecLongName = stream.value("codec_long_name").toString();
            if (codecType == "video") {
                hasVideo = true;
                if (codecLongName.contains("GIF", Qt::CaseInsensitive)) {
                    isGif = true;
                }
                if (formatName.contains("image2") || formatName.contains("mjpeg") || formatName.contains("png") || formatName.contains("jpeg")) {
                    isImage = true;
                }
            }
            if (codecType == "audio") {
                hasAudio = true;
            }
        }
        if (isGif) {
            detectedType = "gif";
        } else if (isImage) {
            detectedType = "image";
        } else if (hasVideo) {
            detectedType = "video";
        } else if (hasAudio) {
            detectedType = "audio";
        }
    }
    // Set media duration for the media player
   // mediaPlayer->setDuration(mediaDuration); // Ensure player gets the duration early
    m_currentMediaDuration = mediaDuration; // Store for waveform processing


    ui->logOutput->append("Detected file type: " + detectedType);

    updateFormatCombo(detectedType);
    showRelevantParams(detectedType); // This will now handle the stacked widget page switch

    // --- Start preview if possible ---
    if (detectedType == "video" || detectedType == "gif") {
        if (m_autoplayEnabled) { // Only play if autoplay is enabled
            mediaPlayer->play();
        }
    } else if (detectedType == "audio") {
        // --- Start FFmpeg process to extract raw audio data for waveform ---
        m_rawAudioData.clear(); // Clear previous data
        if (ffmpegWaveformProcess->state() != QProcess::NotRunning) {
            ffmpegWaveformProcess->kill(); // Kill any previous process
            ffmpegWaveformProcess->waitForFinished();
        }

        QStringList args;
        // Input file
        args << "-i" << ui->inputEdit->text();
        // Output format: raw 16-bit signed little-endian PCM, mono, 8kHz sample rate
        // Using 8kHz and mono to reduce data size for basic visualization
        args << "-f" << "s16le";
        args << "-ac" << "1"; // Mono audio
        args << "-ar" << "8000"; // 8 kHz sample rate
        // Output to pipe (standard output)
        args << "pipe:1";

        ffmpegWaveformProcess->start("ffmpeg", args);
        qDebug() << "Started FFmpeg waveform process with args:" << args;

        if (m_autoplayEnabled) { // Only play if autoplay is enabled
            mediaPlayer->play();
        }
    }
    else if (detectedType == "image") {
        QPixmap pixmap(ui->inputEdit->text());
        if (!pixmap.isNull()) {
            imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            imagePreviewLabel->setText("Failed to load image.");
        }
    }

    probeProcess->deleteLater();
    probeProcess = nullptr;
}

void MainWindow::updateFormatCombo(const QString &type) {
    ui->formatCombo->clear();

    if (type == "video") {
        ui->formatCombo->addItems(MainWindow::SUPPORTED_VIDEO_FORMATS);
    } else if (type == "audio") {
        ui->formatCombo->addItems(MainWindow::SUPPORTED_AUDIO_FORMATS);
    } else if (type == "image") {
        ui->formatCombo->addItems(MainWindow::SUPPORTED_IMAGE_FORMATS);
    } else if (type == "gif") {
        // For GIFs, we want to allow conversion to/from video and common image formats.
        // We'll create a combined list for this case.
        QStringList gifConversionFormats;
        gifConversionFormats << MainWindow::SUPPORTED_VIDEO_FORMATS;
        gifConversionFormats << MainWindow::SUPPORTED_IMAGE_FORMATS;
        gifConversionFormats << MainWindow::SUPPORTED_GIF_FORMATS; // Include gif itself
        gifConversionFormats.removeDuplicates(); // Remove any duplicates if formats overlap
        gifConversionFormats.sort(); // Optional: keep them sorted for consistency
        ui->formatCombo->addItems(gifConversionFormats);

    } else {
        // Fallback for unknown/unhandled types: combine all supported formats
        QStringList allSupportedFormats;
        allSupportedFormats << MainWindow::SUPPORTED_VIDEO_FORMATS;
        allSupportedFormats << MainWindow::SUPPORTED_AUDIO_FORMATS;
        allSupportedFormats << MainWindow::SUPPORTED_IMAGE_FORMATS;
        allSupportedFormats << MainWindow::SUPPORTED_GIF_FORMATS;
        allSupportedFormats.removeDuplicates(); // Essential to avoid multiple entries
        allSupportedFormats.sort(); // Optional: keep them sorted for consistency
        ui->formatCombo->addItems(allSupportedFormats);
    }

    // Existing connect for output file name
    // Disconnect previous connections to avoid multiple triggers on subsequent calls
    disconnect(ui->formatCombo, &QComboBox::currentTextChanged, nullptr, nullptr); // Disconnect all slots from this signal
    connect(ui->formatCombo, &QComboBox::currentTextChanged, this, [this]() {
        QString inputFile = ui->inputEdit->text();
        if (!inputFile.isEmpty()) {
            QString ext = ui->formatCombo->currentText();
            QString outputFile = QFileInfo(inputFile).absolutePath() + "/" +
                                 QFileInfo(inputFile).completeBaseName() + "_converted." + ext;
            ui->outputEdit->setText(outputFile);
        }
    });
}

void MainWindow::showRelevantParams(const QString &type) {
    ui->videoParamsWidget->setVisible(type == "video" || type == "gif");
    ui->audioParamsWidget->setVisible(type == "audio");
    ui->imageParamsWidget->setVisible(type == "image");

    // CRITICAL: Switch the QStackedWidget to the correct preview page
    // And manage visibility of video/audio/image widgets
    if (type == "video" || type == "gif") {
        ui->previewStackedWidget->setCurrentIndex(0); // Page 0 is videoAudioPreviewPage
        ui->mediaControlsWidget->setVisible(true); // Show media controls
        videoWidget->setVisible(true); // Show video widget
        waveformWidget->setVisible(false); // Hide waveform widget
        imagePreviewLabel->clear(); // Clear any previous image
        imagePreviewLabel->setVisible(false); // Hide image label
    } else if (type == "audio") {
        ui->previewStackedWidget->setCurrentIndex(0); // Page 0 is videoAudioPreviewPage
        ui->mediaControlsWidget->setVisible(true); // Show media controls
        videoWidget->setVisible(false); // Hide video widget
        waveformWidget->setVisible(true); // Show waveform widget
        imagePreviewLabel->clear(); // Clear any previous image
        imagePreviewLabel->setVisible(false); // Hide image label
    } else if (type == "image") {
        ui->previewStackedWidget->setCurrentIndex(1); // Page 1 is imagePreviewPage
        ui->mediaControlsWidget->setVisible(false); // Hide media controls
        videoWidget->setVisible(false); // Hide video widget
        waveformWidget->setVisible(false); // Hide waveform widget
        imagePreviewLabel->setVisible(true); // Show image label
    } else { // Unknown type, perhaps show nothing or a default placeholder
        ui->previewStackedWidget->setCurrentIndex(1); // Default to image page
        imagePreviewLabel->setText("No preview available.");
        ui->mediaControlsWidget->setVisible(false);
        videoWidget->setVisible(false);
        waveformWidget->setVisible(false);
        imagePreviewLabel->setVisible(true);
    }

    // Existing visibility logic for play/pause buttons, etc., within mediaControlsWidget:
    bool showMediaControls = (type == "video" || type == "gif" || type == "audio");
    ui->playButton->setVisible(showMediaControls);
    ui->pauseButton->setVisible(showMediaControls);
    ui->stopButton->setVisible(showMediaControls);
    ui->volumeSlider->setVisible(showMediaControls);
    ui->progressBarSlider->setVisible(showMediaControls);
    ui->currentTimeLabel->setVisible(showMediaControls);
    ui->totalTimeLabel->setVisible(showMediaControls);
}

void MainWindow::startConversion() {
    QString inputFile = ui->inputEdit->text();
    if (inputFile.isEmpty()) return;

    QString outputFile = ui->outputEdit->text();
    if (outputFile.isEmpty()) return;

    QStringList args = {"-y", "-i", inputFile};

    // Video parameters
    if (ui->videoParamsWidget->isVisible()) {
        QString res = ui->resolutionCombo->currentText();
        if (!res.isEmpty()) args << "-s" << res;
        int vbitrate = ui->videoBitrateSpin->value();
        if (vbitrate > 0) args << "-b:v" << QString::number(vbitrate) + "k";
    }
    // Audio parameters
    if (ui->audioParamsWidget->isVisible()) {
        int abitrate = ui->audioBitrateSpin->value();
        if (abitrate > 0) args << "-b:a" << QString::number(abitrate) + "k";
        QString sampleRate = ui->sampleRateCombo->currentText();
        if (!sampleRate.isEmpty()) args << "-ar" << sampleRate;
    }
    // Image parameters
    if (ui->imageParamsWidget->isVisible()) {
        int quality = ui->imageQualitySpin->value();
        if (quality > 0) args << "-q:v" << QString::number(quality);
    }

    args << outputFile;

    process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardError, this, &MainWindow::updateLog);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::conversionFinished);

    ui->logOutput->clear();
    ui->progressBar->setValue(0);

    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }

    if (ui->formatCombo->currentText() == "gif") {
        showFPSDialog();
        QStringList args = {"-y", "-i", inputFile};
        QString resolution = ui->resolutionCombo->currentText();
        args << "-vf" << "fps=" + QString::number(fpsToUse) + ",scale=" + resolution;
        args << outputFile;

        process->start("ffmpeg", args);
    } else if (ui->formatCombo->currentText() == "mp4" || ui->formatCombo->currentText() == "avi" ||
               ui->formatCombo->currentText() == "mkv" || ui->formatCombo->currentText() == "mov" ||
               ui->formatCombo->currentText() == "webm") {
        // For video to GIF conversion, we need two steps
        QStringList args = {"-y", "-i", inputFile};

        QString paletteFile = QFileInfo(inputFile).absolutePath() + "/" +
                              QFileInfo(inputFile).completeBaseName() + "_palette.png";
        args << "-vf" << "fps=" + QString::number(fpsToUse) + ",scale=" + ui->resolutionCombo->currentText() + ",palettegen=" << paletteFile;
        process->start("ffmpeg", args);

        // Wait for the first step to complete
        process->waitForFinished();

        args.clear();
        args << "-y";
        args << "-i" << inputFile;
        args << "-i" << paletteFile;
        args << "-filter_complex" << "fps=" + QString::number(fpsToUse) + ",scale=" + ui->resolutionCombo->currentText() + "[x];[x][1:v]paletteuse=dither=bayer";
        args << outputFile;

        process->start("ffmpeg", args);
    }
    else
        process->start("ffmpeg", args);
}

void MainWindow::showFPSDialog() {
    int result = QInputDialog::getInt(this, tr("Select FPS"), tr("Enter the desired frame rate (default: 8):"), 8, 1, 60);
    if (result > 0) {
        fpsToUse = result;
    }
}

void MainWindow::updateLog() {
    QByteArray data = process->readAllStandardError();
    QString text = QString::fromLocal8Bit(data);
    ui->logOutput->append(text);

    // Optional: crude progress increment (you can refine this later)
    if (text.contains("time=")) {
        ui->progressBar->setValue(qMin(ui->progressBar->value() + 1, 99));
    }
}

QString MainWindow::formatFileSize(quint64 bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / GB, 0, 'f', 2); // 2 decimal places
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / KB, 0, 'f', 2);
    } else {
        return QString("%1 Bytes").arg(bytes);
    }
}

QString MainWindow::formatTime(qint64 ms) {
    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}


void MainWindow::conversionFinished(int code, QProcess::ExitStatus status) {
    Q_UNUSED(code);
    Q_UNUSED(status);

    ui->progressBar->setValue(100);
    ui->logOutput->append("Conversion completed.");

    // Clean up the QProcess object
    if (process) {
        process->deleteLater();
        process = nullptr; // Set to nullptr to avoid dangling pointer
    }

    // --- Show a dialog on conversion complete ---
    QMessageBox::information(this, "Conversion Complete", "The Media conversion process has finished successfully!");
}

void MainWindow::getDuration(const QString &filePath) {
    if (QFileInfo(filePath).suffix().isEmpty()) {
        QMessageBox::critical(this, "Error", "Please select a valid audio or video file.");
        return;
    }

    QProcess process;
    QStringList args = {"-v", "error",
                        "-show_entries", "format=duration,size",
                        "-of", "default=noprint_wrappers=1:nokey=0",
                        filePath};
    process.start("ffprobe", args);
    process.waitForFinished();
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        QTextStream stream(&output);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("duration=")) {
                // Extract the duration
                QString cleanedLine = line.trimmed();
                int equalsIndex = cleanedLine.indexOf("=");
                QString durationString = cleanedLine.mid(equalsIndex + 1);
                bool ok;
                double durationDouble = durationString.toDouble(&ok);
                if (ok) {
                    // Convert total seconds into hours, minutes, and seconds
                    int totalSeconds = qRound(durationDouble); // Round to nearest second
                    int hours = totalSeconds / 3600;
                    int minutes = (totalSeconds % 3600) / 60;
                    int seconds = totalSeconds % 60;

                    QString formattedDuration;
                    QString formatHint; // String to hold the format hint (e.g., "(mm:ss)")

                    if (hours > 0) {
                        // If there are hours, include them (hh:mm:ss)
                        formattedDuration = QString("%1:%2:%3")
                                                .arg(hours, 2, 10, QChar('0'))    // 2 digits, decimal, pad with '0'
                                                .arg(minutes, 2, 10, QChar('0'))  // 2 digits, decimal, pad with '0'
                                                .arg(seconds, 2, 10, QChar('0')); // 2 digits, decimal, pad with '0'
                        formatHint = "(hours:minutes:seconds)";
                    } else if (minutes > 0) {
                        // If no hours but there are minutes (mm:ss)
                        formattedDuration = QString("%1:%2")
                                                .arg(minutes, 2, 10, QChar('0'))  // 2 digits, decimal, pad with '0'
                                                .arg(seconds, 2, 10, QChar('0')); // 2 digits, decimal, pad with '0'
                        formatHint = "(minutes:seconds)";
                    } else {
                        // If no hours and no minutes, just show seconds (ss)
                        formattedDuration = QString("%1").arg(seconds); // No padding needed if it's just seconds
                        formatHint = "(seconds)";
                    }

                    // Combine the formatted duration with the format hint
                    ui->durationLabel->setText(formattedDuration + " " + formatHint);
                    qDebug() << "Duration is (double):" << durationDouble;
                    qDebug() << "Formatted Duration:" << formattedDuration;
                    qDebug() << "Format Hint:" << formatHint;
                } else {
                    ui->durationLabel->setText("Duration: N/A");
                    qDebug() << "Error: Could not convert duration string to double:" << durationString;
                }
            } else if (line.startsWith("size=")) {
                QString cleanedLine = line.trimmed();
                int equalsIndex = cleanedLine.indexOf("=");

                if (equalsIndex != -1 && cleanedLine.startsWith("size=")) {
                    QString sizeString = cleanedLine.mid(equalsIndex + 1);
                    bool ok;
                    quint64 fileSize = sizeString.toULongLong(&ok); // Use quint64 for unsigned long long

                    if (ok) {
                        //Get Formatted Size
                        QString formattedSize = formatFileSize(fileSize);
                        ui->sizeLabel->setText(QString("%1").arg(formattedSize));
                        qDebug() << "File size is:" << formattedSize << "(raw:" << fileSize << "bytes)";
                    } else {
                        ui->sizeLabel->setText("N/A");
                        qDebug() << "Error: Could not convert size string to quint64:" << sizeString;
                    }
                }
            }
        }
    } else {
        QMessageBox::critical(this, "Error", "Failed to get the duration and size of the file.");
    }
}


// --- New Media Playback Slots ---

void MainWindow::playMedia() {
    if (mediaPlayer->mediaStatus() == QMediaPlayer::LoadedMedia ||
        mediaPlayer->mediaStatus() == QMediaPlayer::BufferedMedia ||
        mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        mediaPlayer->play();
    }
}

void MainWindow::pauseMedia() {
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer->pause();
    }
}

void MainWindow::stopMedia() {
    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }
}

void MainWindow::mediaStateChanged(QMediaPlayer::PlaybackState state) {
    ui->playButton->setEnabled(state != QMediaPlayer::PlayingState);
    ui->pauseButton->setEnabled(state == QMediaPlayer::PlayingState);
    ui->stopButton->setEnabled(state != QMediaPlayer::StoppedState);
}

void MainWindow::durationChanged(qint64 duration) {
    ui->progressBarSlider->setMaximum(duration);
    ui->totalTimeLabel->setText(formatTime(duration));
    ui->progressBarSlider->setEnabled(true);
}

void MainWindow::positionChanged(qint64 position) {
    ui->progressBarSlider->setValue(position);
    ui->currentTimeLabel->setText(formatTime(position));
}

void MainWindow::setVolume(int volume) {
    audioOutput->setVolume(volume / 100.0); // QAudioOutput volume is 0.0 to 1.0
}

void MainWindow::setPosition(int position) {
    mediaPlayer->setPosition(position);
}

void MainWindow::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
        ui->playButton->setEnabled(true);
        ui->progressBarSlider->setEnabled(true);
    } else if (status == QMediaPlayer::NoMedia || status == QMediaPlayer::InvalidMedia) {
        ui->playButton->setEnabled(false);
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(false);
        ui->progressBarSlider->setEnabled(false);
        ui->currentTimeLabel->setText("00:00");
        ui->totalTimeLabel->setText("00:00");
        if (ui->inputEdit->text().isEmpty()) { // Only show "No Media" if no file is selected
            imagePreviewLabel->setText("No Media Selected");
        }
    }
}


// This slot is called as FFmpeg writes data to stdout
void MainWindow::handleFfmpegWaveformReadyRead() {
    m_rawAudioData.append(ffmpegWaveformProcess->readAllStandardOutput());
    // Optionally, you could do partial processing and update here
    // for very long files, but for initial implementation, accumulate all.
}

// This slot is called when the FFmpeg process finishes
void MainWindow::handleFfmpegWaveformFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "FFmpeg waveform process finished successfully.";

        // Process the accumulated raw audio data
        QVector<qreal> waveformData;
        const int sampleSize = 2; // s16le is 2 bytes per sample
        const int samplesPerPoint = 80; // Number of raw samples to combine into one waveform point
            // Adjust this value based on desired waveform detail vs. performance

        if (m_rawAudioData.size() % sampleSize != 0) {
            qDebug() << "Warning: Raw audio data size is not a multiple of sample size.";
            return;
        }

        for (int i = 0; i < m_rawAudioData.size(); i += sampleSize * samplesPerPoint) {
            qint64 sumAbsolute = 0;
            int count = 0;
            for (int j = 0; j < samplesPerPoint && (i + j * sampleSize + 1) < m_rawAudioData.size(); ++j) {
                // Read 16-bit signed little-endian sample
                qint16 sample = static_cast<qint16>(
                    (static_cast<quint8>(m_rawAudioData[i + j * sampleSize + 1]) << 8) |
                    static_cast<quint8>(m_rawAudioData[i + j * sampleSize])
                    );
                sumAbsolute += qAbs(sample);
                count++;
            }
            if (count > 0) {
                // Normalize the average absolute amplitude to a 0.0-1.0 range
                // Max value for qint16 is 32767
                waveformData.append(static_cast<qreal>(sumAbsolute / count) / 32767.0);
            } else {
                waveformData.append(0.0); // Add a zero if no samples were processed for this point
            }
        }

        // Set the processed waveform data to the widget
        if (!waveformData.isEmpty()) {
            waveformWidget->setWaveformData(waveformData, m_currentMediaDuration);
        } else {
            qDebug() << "Generated empty waveform data.";
            waveformWidget->setWaveformData(QVector<qreal>(), m_currentMediaDuration); // Clear if empty
        }

    } else {
        qWarning() << "FFmpeg waveform process failed with exit code:" << exitCode
                   << "status:" << exitStatus
                   << "Error:" << ffmpegWaveformProcess->readAllStandardError();
        // If FFmpeg fails, clear the waveform
        waveformWidget->setWaveformData(QVector<qreal>(), m_currentMediaDuration);
    }
    m_rawAudioData.clear(); // Clear accumulated data
}

void MainWindow::setAutoplayEnabled(bool enabled) {
    m_autoplayEnabled = enabled;
    qDebug() << "Autoplay enabled set to:" << enabled;
}

bool MainWindow::isAutoplayEnabled() const {
    return m_autoplayEnabled;
}

// Implement the slots to handle signals from UpdateManager
void MainWindow::handleUpdateCheckFinished(bool updateAvailable, const QString &latestVersion) {
    if (updateAvailable) {
        QMessageBox::information(this, "Update Available",
                                 QString("A new version (%1) of %2 is available! Downloading now...").arg(latestVersion).arg(QCoreApplication::applicationName()));
        // Download initiated automatically by UpdateManager
    } else {
        qDebug() << "No update available. Current version:" << latestVersion;
        // QMessageBox::information(this, "No Update", "You are running the latest version.");
    }
}

void MainWindow::handleUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    qDebug() << "Download Progress:" << (bytesReceived * 100 / bytesTotal) << "%";
    // Update a progress bar in your UI here
    // ui->downloadProgressBar->setMaximum(bytesTotal);
    // ui->downloadProgressBar->setValue(bytesReceived);
}

void MainWindow::handleUpdateDownloadFinished(bool success, const QString &errorMessage) {
    if (success) {
        QMessageBox::information(this, "Download Complete", "Update downloaded successfully. Application will now restart to apply update.");
        // App will quit shortly due to QCoreApplication::quit() in UpdateManager
    } else {
        QMessageBox::critical(this, "Download Failed", QString("Update download failed: %1").arg(errorMessage));
    }
}

void MainWindow::handleUpdateInitiated() {
    QMessageBox::information(this, "Applying Update", "Application will now close to apply the update. It will restart automatically.");
    // This message might not be seen long if the app quits immediately.
    // Consider adding a delay or showing a splash screen before quitting.
}

void MainWindow::handleUpdateManagerError(const QString &message) {
    QMessageBox::critical(this, "Update Error", QString("An error occurred during update: %1").arg(message));
    qWarning() << "Update Error:" << message;
}
