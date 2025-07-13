function Component()
{
    // Default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (installer.value("os") === "win") {
        var appName = "QFF Media Converter";
        var appExecutable = "@TargetDir@/qffgui.exe"; // Path to your app executable

        // Create a desktop shortcut
        // Only target executable path and shortcut link path are needed.
        component.addOperation("CreateShortcut",
                               appExecutable,
                               "@DesktopDir@/" + appName + ".lnk"
                               // Removed: description, hotkey, icon path, working directory (if empty)
                              );

        // Create a Start Menu shortcut
        // Only target executable path and shortcut link path are needed.
        component.addOperation("CreateShortcut",
                               appExecutable,
                               "@StartMenuDir@/" + appName + ".lnk"
                               // Removed: description, hotkey, icon path, working directory (if empty)
                              );

        // Important: For the icon to appear, ensure your `qff.exe` has an embedded icon
        // or that a `qff.ico` file is present alongside `qff.exe` in the installation directory.
        // The CreateShortcut operation itself does not take an icon path as an argument.
    }
}
