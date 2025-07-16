import os
import subprocess
import sys
import io
import time
import platform
import webbrowser

# Optional: set your actual GitHub releases page here
GITHUB_RELEASES_URL = "https://github.com/youruser/yourrepo/releases"

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

def log(msg):
    print(f"[Updater] {msg}")

def kill_process(process_name):
    log(f"Killing process: {process_name}")
    try:
        subprocess.run(["taskkill", "/IM", process_name, "/F", "/T"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as e:
        log(f"Failed to kill process {process_name}: {e}")

def run_installer(installer_path):
    log(f"Running installer: {installer_path}")
    try:
        subprocess.run([installer_path], shell=True)  # Waits for installer to finish
        return True
    except Exception as e:
        log(f"Could not run installer: {e}")
        return False

def restart_app(exe_path):
    log(f"Restarting application: {exe_path}")
    try:
        subprocess.Popen([exe_path], shell=True)
    except Exception as e:
        log(f"Could not restart application: {e}")

def show_release_dialog():
    log("Non-Windows platform detected. Showing update link...")
    try:
        # Use platform-native GUI dialog
        if sys.platform.startswith("linux") or sys.platform == "darwin":
            import tkinter as tk
            from tkinter import messagebox

            root = tk.Tk()
            root.withdraw()
            messagebox.showinfo("Update Available",
                                f"Please download the latest update from:\n{GITHUB_RELEASES_URL}",
                                "Or turn off auto-updates in file->update enabled")
        else:
            log(f"Please visit: {GITHUB_RELEASES_URL}")
        # Optional: open in browser
        webbrowser.open(GITHUB_RELEASES_URL)
    except Exception as e:
        log(f"Could not show release dialog: {e}")
        log(f"Please manually visit: {GITHUB_RELEASES_URL}")

def main():
    system = platform.system()
    script_dir = os.path.abspath(os.path.dirname(__file__))

    if system == "Windows":
        # Windows-specific update workflow
        installer_file = None
        for file in os.listdir(script_dir):
            if file.lower().startswith("qffmediaconverter-") and file.lower().endswith(".exe"):
                installer_file = file
                break

        if not installer_file:
            log("Installer not found in directory.")
            return

        installer_path = os.path.join(script_dir, installer_file)
        app_exe_path = os.path.join(script_dir, "qffgui.exe")

        try:
            log("Starting update process...")
            time.sleep(2)

            kill_process("qffgui.exe")

            success = run_installer(installer_path)
            if not success:
                log("Installer failed to launch.")
                return

            restart_app(app_exe_path)

        except Exception as e:
            log(f"Update Failed! Unhandled exception: {e}")

        finally:
            log("Updater finished. Attempting cleanup...")
            try:
                if os.path.exists(installer_path):
                    os.remove(installer_path)
                    log(f"Deleted installer: {installer_file}")
            except Exception as e:
                log(f"Failed to delete installer: {e}")

    else:
        # Non-Windows: show dialog with GitHub release link
        show_release_dialog()

if __name__ == "__main__":
    main()
