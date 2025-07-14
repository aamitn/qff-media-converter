#include "downloadprogressdialog.h"
#include <QApplication> // For QApplication::processEvents() if needed

DownloadProgressDialog::DownloadProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Downloading Update...");
    setModal(true); // Make it a modal dialog so user can't interact with main window

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100); // Percentage
    progressBar->setValue(0);

    statusLabel = new QLabel("Initializing download...", this);
    statusLabel->setAlignment(Qt::AlignCenter);

    cancelButton = new QPushButton("Cancel", this); // Optional
    connect(cancelButton, &QPushButton::clicked, this, &DownloadProgressDialog::cancelDownload);
    cancelButton->hide(); // Hide initially, show if cancellation is allowed

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    layout->addWidget(cancelButton); // Optional
    setLayout(layout);

    setFixedSize(350, 120); // Fixed size for simplicity
}

DownloadProgressDialog::~DownloadProgressDialog() {
    // Widgets are parented to the dialog, so they'll be deleted automatically.
}

void DownloadProgressDialog::updateProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((static_cast<double>(bytesReceived) / bytesTotal) * 100);
        progressBar->setValue(percentage);
        statusLabel->setText(QString("Downloading: %1 MB / %2 MB (%3%)")
                                 .arg(bytesReceived / (1024.0 * 1024.0), 0, 'f', 2)
                                 .arg(bytesTotal / (1024.0 * 1024.0), 0, 'f', 2)
                                 .arg(percentage));
    } else {
        statusLabel->setText(QString("Downloading: %1 MB").arg(bytesReceived / (1024.0 * 1024.0), 0, 'f', 2));
        progressBar->setRange(0, 0); // Indeterminate mode if total size is unknown
    }
    // Ensures the UI updates immediately, especially during heavy downloads
    QApplication::processEvents();
}

void DownloadProgressDialog::setStatusText(const QString &text) {
    statusLabel->setText(text);
    QApplication::processEvents();
}

void DownloadProgressDialog::downloadFinished(bool success, const QString &message) {
    if (success) {
        setStatusText("Download complete! Preparing for update...");
        progressBar->setValue(100);
        // You might close the dialog here or leave it open for a moment
        // and then close it when the update process is initiated.
        // For now, it will remain open until explicitly closed by MainWindow.
    } else {
        setStatusText("Download failed: " + message);
        progressBar->setValue(0);
    }
    // Optionally enable close button or disable cancel button
    // cancelButton->setEnabled(false);
    // You might want to close the dialog automatically after a short delay for success/failure
    // or let the calling code (MainWindow) handle its closure.
}
