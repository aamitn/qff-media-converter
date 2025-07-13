@echo off
rem This script handles the update process. It's launched by the main app.

rem Arguments:
rem %1: Installation directory of the main application (e.g., C:\Program Files\QFFConverter)
rem %2: Temporary directory where the new application files (extracted from update.zip) are located
rem %3: Name of the main application executable (e.g., qffgui.exe)

set "INSTALL_DIR=%~1"
set "TEMP_UPDATE_DIR=%~2"
set "APP_EXE_NAME=%~3"

echo Waiting for main application to close...
timeout /t 5 /nobreak >nul  rem Wait 5 seconds for the app to fully exit

if not exist "%TEMP_UPDATE_DIR%" (
    echo ERROR: Temporary update directory not found: "%TEMP_UPDATE_DIR%"
    goto :end
)

echo Starting update from "%TEMP_UPDATE_DIR%" to "%INSTALL_DIR%"

rem --- Perform the update ---
rem Option 1: Delete old, then copy new (less safe but simpler)
rem This can fail if files are still locked.
rem rmdir /s /q "%INSTALL_DIR%\data"
rem xcopy /s /e /y "%TEMP_UPDATE_DIR%\data\*" "%INSTALL_DIR%\data\"
rem xcopy /s /e /y "%TEMP_UPDATE_DIR%\*.exe" "%INSTALL_DIR%\"
rem etc.

rem OPTION 2: Safer update (rename old, copy new, then delete old)
set "BACKUP_DIR=%INSTALL_DIR%_backup_%RANDOM%"
echo Creating backup of old installation: "%BACKUP_DIR%"
move "%INSTALL_DIR%" "%BACKUP_DIR%"

echo Copying new application files...
xcopy /s /e /y "%TEMP_UPDATE_DIR%\*" "%INSTALL_DIR%\"

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to copy new files. Attempting to restore backup.
    rem Restore backup if copying failed
    rmdir /s /q "%INSTALL_DIR%"
    move "%BACKUP_DIR%" "%INSTALL_DIR%"
    goto :end
)

rem Clean up backup after successful copy
echo Cleaning up backup directory: "%BACKUP_DIR%"
rmdir /s /q "%BACKUP_DIR%"

rem --- Cleanup temporary update files ---
echo Cleaning up temporary update directory: "%TEMP_UPDATE_DIR%"
rmdir /s /q "%TEMP_UPDATE_DIR%"

echo Update successful. Restarting application...
start "" "%INSTALL_DIR%\%APP_EXE_NAME%"

:end
rem Delete this update script itself after execution (optional)
del "%~f0"
exit