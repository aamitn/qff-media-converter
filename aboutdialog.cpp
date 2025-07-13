#include "aboutdialog.h"
#include "ffmpegutils.h"

#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QPixmap>
#include <QDate>
#include <QRegularExpression>
#include <QtGlobal>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("About QFF Media Converter");
    setMinimumSize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Load and show icon from resources
    QLabel *iconLabel = new QLabel(this);
    QPixmap iconPixmap(":/icon/6.png");
    iconLabel->setPixmap(iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(iconLabel);

    // Get current year dynamically
    int currentYear = QDate::currentDate().year(); // Ensure QDate is included if not already

    // 1. Get the full FFmpeg version output
    QString fullFfmpegOutput = FFMpegUtils::getFfmpegVersion();
    QString ffmpegVersionToDisplay = "Unknown"; // Default in case parsing fails

    // 2. Use QRegularExpression to extract the version number
    QRegularExpression re("ffmpeg version\\s+(\\S+)");
    QRegularExpressionMatch match = re.match(fullFfmpegOutput);

    if (match.hasMatch()) {
        ffmpegVersionToDisplay = match.captured(1); // Get the captured group (the version string)
    } else {
        // If parsing fails, you can log the full output for debugging
        qWarning() << "Could not parse FFmpeg version from output. Full output:\n" << fullFfmpegOutput;
        // Optionally, you might want to show a truncated version of the full output
        // if the parsing fails for debugging or user info.
        // ffmpegVersionToDisplay = fullFfmpegOutput.left(50) + "...";
    }

    // 3. Use the extracted version in your QLabel
    QLabel *titleLabel = new QLabel(
        QString("<b>QFF Media Converter</b> (Version: %1)<br>"
                "Powered by <b>Qt</b> (Version: %4) and <b>FFmpeg</b> (Version: %3)<br>" // <--- Added %4 for Qt Version
                "\u00A9 %2 Bitmutex Technologies")
            .arg(APP_VERSION)
            .arg(currentYear)
            .arg(ffmpegVersionToDisplay)
            .arg(qVersion()), // <--- Added qVersion() here as the fourth argument
        this
        );
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QTextEdit *licenseEdit = new QTextEdit(this);
    licenseEdit->setReadOnly(true);
    licenseEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    QString licenseText = R"(MIT License
Copyright (c) %1 Bitmutex Technologies
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";
    licenseEdit->setPlainText(licenseText.arg(currentYear));
    mainLayout->addWidget(licenseEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *repoButton = new QPushButton("Repository", this);
    QPushButton *websiteButton = new QPushButton("Website", this);
    QPushButton *donateButton = new QPushButton("Donate", this);
    QPushButton *closeButton = new QPushButton("Close", this);

    connect(repoButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/aamitn/qff"));
    });
    connect(websiteButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://bitmutex.com/qffgui"));
    });
    connect(donateButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://bitmutex.com/donate"));
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(repoButton);
    buttonLayout->addWidget(websiteButton);
    buttonLayout->addWidget(donateButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}
