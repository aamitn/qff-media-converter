#include "ffmpegstatusdialog.h"
#include "ui_ffmpegstatusdialog.h"

FfmpegStatusDialog::FfmpegStatusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FfmpegStatusDialog)
{
    ui->setupUi(this);
    ui->progressBar->setRange(0, 0); // Indeterminate
}

FfmpegStatusDialog::~FfmpegStatusDialog()
{
    delete ui;
}

void FfmpegStatusDialog::setStatus(const QString &status)
{
    ui->labelStatus->setText(status);
    qApp->processEvents(); // Update UI immediately
}
