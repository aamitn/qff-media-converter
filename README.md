# QFF Media Converter

<img src=".\icon\6.png" alt="QFF Logo" width="200" align="left"/>

[![Build App](https://github.com/aamitn/qff-media-converter/actions/workflows/cmake.yml/badge.svg)](https://github.com/aamitn/qff-media-converter/actions/workflows/cmake.yml)
[![License](https://img.shields.io/github/license/aamitn/qff-media-converter)](./LICENSE)
[![Version](https://img.shields.io/github/v/release/aamitn/qff-media-converter)](https://github.com/aamitn/Winhider/releases/)


| **Release Type**    | **Link**                                                                                                                                                                                            |
|---------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Winget**          | **`winget install qffgui`**                                                                                                                                                                       |
| **Github Release**  | [Installer Link](https://github.com/aamitn/winhider/releases/download/v0.8.15/WinhiderInstaller.exe) • [Zip Bundle Link](https://github.com/aamitn/winhider/releases/download/v0.8.15/Winhider.zip) |
| **Microsoft Store** | [Store Link](https://www.xbox.com/en-IN/auth/msa?action=logIn&returnUrl=%2Fen-IN%2Fgames%2Fstore%2Fdoom-the-dark-ages%2F9ph9x0760b0t&prompt=none)                                                   |



**QFF Media Converter** is a lightweight, Qt-based front-end application that simplifies media conversion tasks using the powerful FFmpeg engine. Developed by [Bitmutex Technologies](https://www.bitmutex.com), the tool provides an intuitive UI for converting audio, image  and video files from `any`-to-`any` format.

Website Repo for docs and landing : [`https://github.com/aamitn/qff-media-converter`](https://github.com/aamitn/qff-media-converter)
[![GH Pages Deploy](https://github.com/aamitn/winhider-website/actions/workflows/astro.yml/badge.svg)](https://github.com/aamitn/winhider-website/actions/workflows/astro.yml)


## Glimpse of the GUI and CLI
<p float="left">
  <img src="./icon/ss.png" width="400" alt="Winhider GUI" />
</p>
---

## 🚀 Features

- 🎞️ Convert audio and video and image files from `any`-to-`any` formats using FFmpeg
- 📈 Visualize audio waveforms in real-time
- 🔄 Integrated auto-updater with support for GitHub releases
- 📦 FFmpeg auto-download and installation
- 🐍 Python-based update helper script for smooth file replacement
- 🪟 Cross-platform (Windows & Linux), with dynamic script handling
- 🌍 Internationalization support via `.ts` translation files
- 🧰 Minimal external dependencies (uses system libraries only)

---

## 🧑‍💻 Getting Started

### ⚙️ Prerequisites

- Qt 6.x (Tested with Qt 6.9.1)
- CMake 3.16+
- Python 3.8+ (auto-installed if not found)
- Git (optional for CI/CD or updates from GitHub)

---

### 🔨 Build Instructions

#### Using CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

#### Using Qt Creator

- Open `CMakeLists.txt` in Qt Creator.
- Configure the kit and run the project.
- Build and Run in IDE


## 📦  FFmpeg Integration

If FFmpeg is not found on the user's system, the application automatically downloads and installs it from a pre-defined location. This logic is handled in:
- `ffmpegutils.cpp`
- `unzip_ffmpeg.py`

##  🔄 Auto-Update Mechanism

The update manager checks GitHub releases and downloads .zip assets, which are extracted and installed via:
    - `update_helper.py`: a cross-platform update script written in Python.
    -`updatemanager.cpp`: integrates with GitHub's REST API.
    - Automatically restarts the app after copying updated files.
>Auto-Updates can e tunrned off from UI Menu

💬 Translations

Translation support via Qt Linguist .ts files is available. Contributions welcome.


🤝 Contributing
**License** :: This project is licensed under the MIT License.

Pull requests and feature suggestions are welcome! For major changes, please open an issue first.