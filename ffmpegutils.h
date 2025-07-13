#ifndef FFMPEGUTILS_H
#define FFMPEGUTILS_H

#include <QString>
#include <QProcessEnvironment>

namespace FFMpegUtils {
bool isFfmpegAvailable(QString *errorMsg = nullptr);
bool downloadAndInstallFfmpeg(QString *errorMsg = nullptr);
void addFfmpegToPath(const QString &ffmpegBinDir);
QString getFfmpegPath();
QString getFfmpegVersion();
}

#endif // FFMPEGUTILS_H
