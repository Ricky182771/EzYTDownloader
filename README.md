# EzYTDownloader 

<img src="https://github.com/Ricky182771/EzYTDownloader/blob/main/resources/icons/app_icon.png" width="100" height="100">

A native Linux desktop application designed for downloading and converting YouTube content. The project focuses on performance, modularity, and resource efficiency, entirely avoiding the use of web-based frameworks.

## Overview
EzYTDownloader provides a graphical interface for stream extraction and media conversion. It invokes `yt-dlp` directly as a system command — no Python runtime is required. All downloads are processed through ffmpeg to ensure broad compatibility of the final files.

## Features
**Native UI:** Developed with Qt6 to offer a smooth and lightweight desktop experience.

**Video Formats:** Allows downloading and remuxing data streams into standard MP4 and MKV containers.

**Audio Extraction:** Direct conversion to MPEG (MP3) format.

**Playlists & Parallel Downloads:** Detects YouTube playlists automatically and downloads their entries concurrently (1–5 simultaneous downloads, configurable), with a per-item progress table showing title, progress, speed, and ETA.

**Asynchronous Processing:** Non-blocking interface with real-time tracking of progress, speed, and estimated time of arrival (ETA).

**Zero Python dependency:** Communicates with `yt-dlp` directly via its CLI, keeping the dependency footprint minimal.

## Architecture
The application is structured into two main layers:

| Layer | Technology | Responsibility |
|-------|------------|----------------|
| **UI & Core** | **C++20 / Qt6** | Main window, orchestration of asynchronous processes (QProcess), metadata parsing, and state management. |
| **Conversion** | **ffmpeg** | Remuxing of native formats (e.g., .webm) to final .mp4, .mkv, or .mp3 files. |

`yt-dlp` is invoked as an external command for fetching video metadata (`--dump-json`) and downloading streams.

## Prerequisites

The dependencies fall into two groups: those needed to **build** the application, and those needed to **run** it. Install both before proceeding.

### Runtime dependencies

These external tools are invoked by the application at runtime and must be present on your `PATH`. The app checks for them on startup and shows a dependency dialog if any are missing.

| Dependency | Debian / Ubuntu | Fedora | Arch Linux | openSUSE | Notes |
|------------|-----------------|--------|------------|----------|-------|
| yt-dlp | `yt-dlp` | `yt-dlp` | `yt-dlp` | `yt-dlp` | Keep it up to date (`yt-dlp -U` or your package manager) — YouTube changes often break older releases. |
| ffmpeg | `ffmpeg` | `ffmpeg` | `ffmpeg` | `ffmpeg` | Used for remuxing/conversion to the final container. |
| git | `git` | `git` | `git` | `git` | Only needed for the `git pull` step of the install script. |

### Build dependencies

| Dependency | Debian / Ubuntu | Fedora | Arch Linux | openSUSE |
|------------|-----------------|--------|------------|----------|
| CMake (≥ 3.20) | `cmake` | `cmake` | `cmake` | `cmake` |
| C++20 compiler | `g++` | `gcc-c++` | `gcc` | `gcc-c++` |
| Qt6 (Core, Widgets, Concurrent, Network) | `qt6-base-dev` `libqt6concurrent6` `libqt6network6` | `qt6-qtbase-devel` | `qt6-base` | `qt6-base-devel` |

## Installation and Execution

### Quick install (recommended)

After installing the build and runtime dependencies above, clone the repository and run the install script. It performs the whole pipeline in one step — **updates the source (`git pull`), compiles, and installs** the binary, desktop entry, and icon system-wide.

```bash
git clone https://github.com/Ricky182771/EzYTDownloader.git
cd EzYTDownloader
bash scripts/install.sh
```

> Run it as a **normal user** — *not* with `sudo`. The build must not run as root; the script asks for `sudo` by itself only for the system install steps.

Re-running `bash scripts/install.sh` later pulls the latest changes, rebuilds, and reinstalls — the simplest way to stay on the newest version.

Useful flags:

| Flag | Effect |
|------|--------|
| `--no-pull`  | Skip `git pull`; build and install the current checkout as-is. |
| `--no-build` | Skip compilation; install the binary already present in `build/`. |

### Building from Source (manual)

If you prefer to compile by hand instead of using the script:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The compiled binary is then at `build/EzYTDownloader`. You can run it directly, or install it system-wide with `bash scripts/install.sh --no-pull --no-build`.

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
