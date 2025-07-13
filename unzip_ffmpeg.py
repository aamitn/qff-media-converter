# unzip_ffmpeg.py
import sys
import zipfile
import os
import ctypes

def broadcast_environment_change():
    HWND_BROADCAST = 0xFFFF
    WM_SETTINGCHANGE = 0x001A
    SMTO_ABORTIFHUNG = 0x0002
    ctypes.windll.user32.SendMessageTimeoutW(
        HWND_BROADCAST,
        WM_SETTINGCHANGE,
        0,
        "Environment",
        SMTO_ABORTIFHUNG,
        5000,
        None
    )


def add_to_user_path(bin_dir):
    import winreg
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Environment', 0, winreg.KEY_READ | winreg.KEY_SET_VALUE) as key:
        try:
            current_path, _ = winreg.QueryValueEx(key, 'Path')
        except FileNotFoundError:
            current_path = ""
        if bin_dir not in current_path:
            new_path = bin_dir + ";" + current_path
            winreg.SetValueEx(key, 'Path', 0, winreg.REG_EXPAND_SZ, new_path)
            print(f"Added {bin_dir} to user PATH.")
            broadcast_environment_change()  # <-- Broadcast after updating PATH
        else:
            print(f"{bin_dir} already in user PATH.")
            broadcast_environment_change()  # <-- Still broadcast to ensure update

def main(zip_path, dest_dir):
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(dest_dir)
    # Find ffmpeg/bin
    bin_dir = ""
    for root, dirs, files in os.walk(dest_dir):
        if os.path.basename(root).lower() == "bin" and "ffmpeg.exe" in files:
            bin_dir = root
            break
    if not bin_dir:
        print("Could not find ffmpeg/bin directory after extraction.", file=sys.stderr)
        sys.exit(1)
    add_to_user_path(bin_dir)
    print("Extraction and PATH update complete.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python unzip_ffmpeg.py <zip_path> <dest_dir>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])