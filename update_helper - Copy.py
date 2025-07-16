import os
import shutil
import subprocess
import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')
import time
import zipfile
import platform

def log(msg):
    print(f"[Updater] {msg}")

def unzip_update(zip_path, extract_to):
    log(f"Unzipping: {zip_path} → {extract_to}")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        log("Unzip successful.")
        return True
    except Exception as e:
        log(f"Failed to unzip: {e}")
        return False

def kill_process(process_name):
    log(f"Killing process: {process_name}")
    try:
        subprocess.run(["taskkill", "/IM", process_name, "/F", "/T"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as e:
        log(f"Failed to kill process {process_name}: {e}")

def copy_dist_files(dist_dir, target_dir):
    log(f"Copying files from {dist_dir} to {target_dir}...")
    for root, dirs, files in os.walk(dist_dir):
        for file in files:
            src_file = os.path.join(root, file)
            rel_path = os.path.relpath(src_file, dist_dir)
            dest_file = os.path.join(target_dir, rel_path)

            os.makedirs(os.path.dirname(dest_file), exist_ok=True)
            shutil.copy2(src_file, dest_file)
            log(f"→ {rel_path}")
    log("File copy complete.")

def restart_app(exe_path):
    log(f"Restarting application: {exe_path}")
    try:
        subprocess.Popen([exe_path], shell=True)
    except Exception as e:
        log(f"Could not restart application: {e}")

def main():
    script_dir = os.path.abspath(os.path.dirname(__file__))
    zip_path = os.path.join(script_dir, "update.zip")
    dist_dir = os.path.join(script_dir, "dist")
    # Detect OS and set app executable path accordingly
    if platform.system() == "Windows":
        app_exe = os.path.join(script_dir, "qffgui.exe")
    else:
        app_exe = os.path.join(script_dir, "qffgui")  # Unix-like systems

    try:
        log("Starting update process...")

        # Wait briefly to allow main app to exit
        time.sleep(2)

        # Step 1: Unzip
        if not unzip_update(zip_path, script_dir):
            log("Aborting update due to unzip failure.")
            return

        # Step 2: Kill running process
        kill_process("qffgui.exe")

        # Step 3: Copy updated files
        copy_dist_files(dist_dir, script_dir)

        # Step 4: Restart the app
        success = restart_app(app_exe)
        if not success:
            log("App failed to restart.")
            return

    except Exception as e:
        log(f"Update Failed! Unhandled exception during update: {e}")

    finally:
        # Step 5: Cleanup if app was restarted
        if os.path.exists(zip_path):
            try:
                os.remove(zip_path)
                log("Deleted update.zip")
            except Exception as e:
                log(f"Failed to delete update.zip: {e}")

        if os.path.exists(dist_dir):
            try:
                shutil.rmtree(dist_dir)
                log("Deleted dist directory")
            except Exception as e:
                log(f"Failed to delete dist directory: {e}")

if __name__ == "__main__":
    main()
