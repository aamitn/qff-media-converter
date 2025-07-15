#ifndef PYTHONINSTALLER_H
#define PYTHONINSTALLER_H

#include <QObject>
#include <QString>

#define PYTHON_VERSION "3.10.11"
#define PYTHON_INSTALLER_NAME "python-" PYTHON_VERSION "-amd64.exe"
#define PYTHON_DOWNLOAD_URL "https://www.python.org/ftp/python/" PYTHON_VERSION "/" PYTHON_INSTALLER_NAME

class PythonInstaller : public QObject
{
    Q_OBJECT

public:
    explicit PythonInstaller(QObject *parent = nullptr);

    // Check if Python is already available (python3 or python)
    bool isPythonAvailable();

    // Download installer (only on Windows)
    bool downloadInstaller(const QString &savePath);

    // Install silently based on platform
    bool installSilently(const QString &installerPath);

    // Returns the detected python executable ("python" or "python3")
    QString pythonExecutable() const;

private:
    QString detectedExecutable;
};

#endif // PYTHONINSTALLER_H
