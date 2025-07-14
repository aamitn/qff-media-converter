#ifndef DOWNLOADPROGRESSDIALOG_H
#define DOWNLOADPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton> // Optional: for a cancel button

class DownloadProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadProgressDialog(QWidget *parent = nullptr);
    ~DownloadProgressDialog();

public slots:
    void updateProgress(qint64 bytesReceived, qint64 bytesTotal);
    void setStatusText(const QString &text);
    void downloadFinished(bool success, const QString &message = QString());

signals:
    void cancelDownload(); // Emitted if a cancel button is pressed

private:
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QPushButton *cancelButton; // Optional
};

#endif // DOWNLOADPROGRESSDIALOG_H
