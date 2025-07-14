#include "menubarhelper.h"
#include "ffmpegutils.h"
#include "mainwindow.h" // Needed for MainWindow access
#include "aboutdialog.h"

#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QProcess>

// --- NEW: Function definition takes MainWindow* ---
QMenuBar* MenuBarHelper::createMenuBar(MainWindow *mainWindow) {

    if (!mainWindow) {
        qWarning() << "MenuBarHelper: Parent is not a MainWindow instance. Cannot setup update action.";
        return nullptr;
    }

    QMenuBar *menuBar = new QMenuBar(mainWindow); // Use mainWindow as parent for menuBar

    QMenu *fileMenu = menuBar->addMenu("&File");

    QAction *openFileAction = fileMenu->addAction("Open File");
    QObject::connect(openFileAction, &QAction::triggered, mainWindow, &MainWindow::browseFile);
    openFileAction->setShortcut(QKeySequence("Ctrl+O"));

    // --- Enable Autoplay Action ---
    QAction *enableAutoplayAction = fileMenu->addAction("Enable Autoplay");
    enableAutoplayAction->setCheckable(true);
    // Set initial state based on MainWindow's current setting
    enableAutoplayAction->setChecked(mainWindow->isAutoplayEnabled());
    QObject::connect(enableAutoplayAction, &QAction::toggled, mainWindow, &MainWindow::setAutoplayEnabled);

    // --- Update Action ---
    QAction *autoUpdateAction = fileMenu->addAction("Auto-Update on Startup"); // More descriptive text
    autoUpdateAction->setCheckable(true);
    // Set the initial checked state from MainWindow's loaded setting
    autoUpdateAction->setChecked(mainWindow->isUpdateEnabled());
    QObject::connect(autoUpdateAction, &QAction::toggled, mainWindow, &MainWindow::setUpdateEnabled);



    fileMenu->addSeparator(); // Add a separator for better organization

    QAction *showFfmpegPathAction = fileMenu->addAction("Show FFmpeg Path");
    QObject::connect(showFfmpegPathAction, &QAction::triggered, mainWindow, [mainWindow]() {
        QString ffmpegPath = FFMpegUtils::getFfmpegPath();
        QMessageBox::information(mainWindow, "FFmpeg Path", ffmpegPath);
    });
    showFfmpegPathAction->setShortcut(QKeySequence("Ctrl+Shift+P"));

    QAction *showFfmpegVersionAction = fileMenu->addAction("Show FFmpeg Version");
    QObject::connect(showFfmpegVersionAction, &QAction::triggered, mainWindow, [mainWindow]() {
        QString version = FFMpegUtils::getFfmpegVersion();
        QMessageBox::information(mainWindow, "FFmpeg Version", version);
    });
    showFfmpegVersionAction->setShortcut(QKeySequence("Ctrl+Shift+V"));

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit");
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QWidget::close);

    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    QObject::connect(aboutAction, &QAction::triggered, mainWindow, [mainWindow]() {
        AboutDialog dialog(mainWindow);
        dialog.exec();
    });

    return menuBar;
}
