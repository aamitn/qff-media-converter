#pragma once
#ifndef MENUBARHELPER_H
#define MENUBARHELPER_H

#include <QMenuBar>
#include <QWidget>

// --- forward declaration ---
class MainWindow;

class MenuBarHelper {
public:
    static QMenuBar* createMenuBar(MainWindow *mainWindow);
};

#endif // MENUBARHELPER_H
