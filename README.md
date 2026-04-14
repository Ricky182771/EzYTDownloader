# EzYTDownloader 

<img src="https://github.com/Ricky182771/EzYTDownloader/blob/main/resources/icons/app_icon.png" width="100" height="100">

A native Linux desktop application designed for downloading and converting YouTube content. The project focuses on performance, modularity, and resource efficiency, entirely avoiding the use of web-based frameworks.

## Overview
EzYTDownloader provides a graphical interface for stream extraction and media conversion. It utilizes a hybrid architecture that combines the speed of a C++/Qt6 frontend with a Python backend for metadata management. All downloads are processed through ffmpeg to ensure broad compatibility of the final files.

## Features
**Native UI:** Developed with Qt6 to offer a smooth and lightweight desktop experience.

**Video Formats:** Allows downloading and remuxing data streams into standard MP4 and MKV containers.

**Audio Extraction:** Direct conversion to MPEG (MP3) format.

**Asynchronous Processing:** Non-blocking interface with real-time tracking of progress, speed, and estimated time of arrival (ETA).

**Modular Architecture:** Strict separation of responsibilities through JSON-lines-based IPC between the C++ core and the Python extraction wrapper.

## Architecture
The application is structured into three main layers:

| Layer | Technology | Responsibility |
|-------|------------|----------------|
| **UI & Core** | **C++20 / Qt6** | Main window, orchestration of asynchronous processes (QProcess), and state management. |
| **Backend** | **Python 3** | Wrapper for yt-dlp responsible for fetching metadata and managing downloads via a JSON-lines protocol. |
| **Conversion** | **ffmpeg** | Remuxing of native formats (e.g., .webm) to final .mp4, .mkv, or .mp3 files. |

## Prerequisites
Ensure the following dependencies are installed before proceeding with compilation:

### Runtime dependencies
- Python 3
- yt-dlp
- ffmpeg

### Build dependencies

| Dependency | Debian / Ubuntu | Fedora | Arch Linux | openSUSE |
|------------|-----------------|--------|------------|----------|
| CMake (≥ 3.20) | `cmake` | `cmake` | `cmake` | `cmake` |
| C++20 compiler | `g++` | `gcc-c++` | `gcc` | `gcc-c++` |
| Qt6 (Core, Widgets, Concurrent, Network) | `qt6-base-dev` `libqt6concurrent6` `libqt6network6` | `qt6-qtbase-devel` | `qt6-base` | `qt6-base-devel` |

> **Tip:** You can use `scripts/install_deps.sh` to auto-install runtime dependencies (python3, ffmpeg, yt-dlp).

## Installation and Execution

### Precompiled Binaries
For users who prefer not to build from source, the final executable is available in the **Releases** section of this repository.

### Building from Source
To manually compile the application:

```bash
# Clone the repository
git clone https://github.com/Ricky182771/EzYTDownloader.git
cd EzYTDownloader

# Create the build directory
mkdir -p build && cd build

# Configure and compile
cmake ..
make -j$(nproc)
```

### System Installation
If you downloaded only the binary, you can install to your system by using the provided install script to register the binary, desktop entry, and application icon system-wide. **This step requires `sudo`.**

```bash
# Install (binary → /usr/local/bin, desktop entry, icon)
sudo bash scripts/install.sh
```

You can then launch **EzYTDownloader** from your application menu or directly from the terminal.

### Uninstalling
To remove all installed files:

```bash
sudo bash scripts/uninstall.sh
```

The script will ask for confirmation before deleting anything.

## Usage

1. Launch EzYTDownloader.

2. Paste the video URL into the input field and click Fetch to retrieve available formats.

3. Select the desired container (MP4, MKV, or MP3) and the target resolution.

4. Define the output directory.

5. Click Download. The application will automatically handle the download and subsequent conversion.

## License
This project is distributed under the GNU General Public License (GPL). See the LICENSE file for further details.
