#!/bin/bash
# This script handles the update process for Linux/macOS.
# It's launched by the main app and receives arguments.

# Arguments:
# $1: Installation directory of the main application (e.g., /opt/qffconverter or /Applications/QFFConverter.app)
# $2: Temporary directory where the new application files (extracted from update.zip) are located
# $3: Path to the downloaded zip file (e.g., /tmp/tempdir/update.zip)
# $4: Application Name (e.g., qffgui)
# $5: Full executable name (e.g., qffgui or QFFConverter.app/Contents/MacOS/QFFConverter)

INSTALL_DIR="$1"
TEMP_UPDATE_DIR="$2"
ZIP_FILE_PATH="$3"
APP_NAME="$4"
APP_EXE_PATH="$5" # This should be the full path to the executable inside the bundle/dir

echo "Waiting for main application to close..."
sleep 5 # Wait 5 seconds for the app to fully exit

if [ ! -d "$TEMP_UPDATE_DIR" ]; then
    echo "ERROR: Temporary update directory not found: $TEMP_UPDATE_DIR"
    exit 1
fi

echo "Starting update from $TEMP_UPDATE_DIR to $INSTALL_DIR"

# Extract the update zip to a temporary location for installation
EXTRACTED_UPDATE_DIR="$TEMP_UPDATE_DIR/extracted_update"
mkdir -p "$EXTRACTED_UPDATE_DIR"

echo "Extracting $ZIP_FILE_PATH to $EXTRACTED_UPDATE_DIR..."
# You might need 'unzip' installed on the system, or bundle a static unzip binary.
# Or, the Qt app could handle unzipping before launching this script.
unzip -o "$ZIP_FILE_PATH" -d "$EXTRACTED_UPDATE_DIR"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to unzip update file."
    rm -rf "$EXTRACTED_UPDATE_DIR"
    exit 1
fi

# OPTION 1: Safer update (rename old, copy new, then delete old)
BACKUP_DIR="${INSTALL_DIR}_backup_$(date +%s%N)"
echo "Creating backup of old installation: ${BACKUP_DIR}"
mv "$INSTALL_DIR" "$BACKUP_DIR"

echo "Copying new application files..."
cp -r "$EXTRACTED_UPDATE_DIR"/* "$INSTALL_DIR"/

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to copy new files. Attempting to restore backup."
    # Restore backup if copying failed
    rm -rf "$INSTALL_DIR"
    mv "$BACKUP_DIR" "$INSTALL_DIR"
    exit 1
fi

# Clean up backup after successful copy
echo "Cleaning up backup directory: ${BACKUP_DIR}"
rm -rf "$BACKUP_DIR"

# --- Cleanup temporary update files ---
echo "Cleaning up temporary update directory: ${TEMP_UPDATE_DIR}"
rm -rf "$TEMP_UPDATE_DIR"

echo "Update successful. Restarting application..."
# Depending on your platform and how you start it, this path might need adjustment.
# For macOS bundles, it's typically 'open /Applications/YourApp.app'
# For Linux, it's just the executable path.
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: Assuming app is in /Applications or similar, and APP_EXE_PATH points to YourApp.app
    open "$INSTALL_DIR"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux: Launch the executable directly
    "$INSTALL_DIR/$APP_EXE_PATH" & # & to run in background
else
    # Fallback or other OS
    echo "Unknown OS, trying to start: $INSTALL_DIR/$APP_EXE_PATH"
    "$INSTALL_DIR/$APP_EXE_PATH" &
fi


# Delete this update script itself after execution (optional)
rm -- "$0"
exit 0