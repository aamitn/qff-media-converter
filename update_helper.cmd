@echo off
setlocal enabledelayedexpansion

REM Get the directory where this script is located at
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Unzip the update archive
python "%SCRIPT_DIR%unzip_ffmpeg.py" "%SCRIPT_DIR%update.zip" "%SCRIPT_DIR%."

REM Kill the running instance (ignore errors if not running)
taskkill /IM "qffgui.exe" /F /T >nul 2>&1

REM Echo status message
echo Copying files to application directory...

REM Recursively copy files from dist to script dir
for /r "%SCRIPT_DIR%dist" %%F in (*) do (
    set "SRC=%%F"
    set "DEST=%%F"
    set "DEST=!DEST:%SCRIPT_DIR%dist=%SCRIPT_DIR%!"
    mkdir "!DEST!\.." >nul 2>&1
    copy /y "!SRC!" "!DEST!" >nul
)

REM Optionally restart app
start "" "%SCRIPT_DIR%qffgui.exe"

endlocal
