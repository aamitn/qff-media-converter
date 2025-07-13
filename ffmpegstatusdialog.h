#ifndef FFMPEGSTATUSDIALOG_H
#define FFMPEGSTATUSDIALOG_H

#include <QDialog>

namespace Ui {
class FfmpegStatusDialog;
}

class FfmpegStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FfmpegStatusDialog(QWidget *parent = nullptr);
    ~FfmpegStatusDialog();

    void setStatus(const QString &status);

private:
    Ui::FfmpegStatusDialog *ui;
};

#endif // FFMPEGSTATUSDIALOG_H
