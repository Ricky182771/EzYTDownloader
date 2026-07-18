#!/usr/bin/env bash
# ============================================================================
# install.sh — Update, build and install EzYTDownloader system-wide.
#
# Full pipeline (from the repository root):
#   1. git pull   — fetch the latest source            (skip with --no-pull)
#   2. cmake+make — configure and compile              (skip with --no-build)
#   3. install    — binary, desktop entry and icon     (uses sudo)
#
# Run it as a NORMAL user (not with sudo): the build must not run as root,
# and the script escalates by itself only for the system install steps.
#
#   Usage:
#     bash scripts/install.sh [--no-pull] [--no-build]
# ============================================================================
set -euo pipefail

# ── Colors ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }

# ── Parse arguments ──────────────────────────────────────────────────────────
DO_PULL=1
DO_BUILD=1
for arg in "$@"; do
    case "$arg" in
        --no-pull)  DO_PULL=0 ;;
        --no-build) DO_BUILD=0 ;;
        -h|--help)
            echo "Usage: bash scripts/install.sh [--no-pull] [--no-build]"
            echo "  --no-pull   Do not run 'git pull' before building."
            echo "  --no-build  Do not compile; install the existing build/ binary."
            exit 0 ;;
        *)
            error "Unknown option: $arg"
            echo  "Try: bash scripts/install.sh --help"
            exit 1 ;;
    esac
done

# ── Resolve project root (directory containing CMakeLists.txt) ───────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
    error "Cannot find CMakeLists.txt in $PROJECT_ROOT"
    error "Please run this script from the repository root or from scripts/."
    exit 1
fi

# ── Paths ────────────────────────────────────────────────────────────────────
APP_NAME="EzYTDownloader"
BUILD_DIR="$PROJECT_ROOT/build"
BINARY="$BUILD_DIR/$APP_NAME"

PREFIX="/usr/local"
BIN_DIR="$PREFIX/bin"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/256x256/apps"

# ── Escalation helper: prefix install commands with sudo only if needed ──────
if [[ $EUID -eq 0 ]]; then
    SUDO=""
    warn "Running as root. Building as root is discouraged; prefer:"
    warn "    bash scripts/install.sh"
else
    SUDO="sudo"
fi

# ── Banner ───────────────────────────────────────────────────────────────────
echo -e "${CYAN}${BOLD}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║        EzYTDownloader — Update · Build · Install         ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ── 1. Update source (git pull) ──────────────────────────────────────────────
if [[ $DO_PULL -eq 1 ]]; then
    if [[ -d "$PROJECT_ROOT/.git" ]] && command -v git &>/dev/null; then
        info "Updating source (git pull --ff-only)..."
        if git -C "$PROJECT_ROOT" pull --ff-only; then
            success "Source updated to the latest revision."
        else
            warn "git pull failed (local changes, no remote, or diverged history)."
            warn "Continuing with the current source. Use --no-pull to silence this."
        fi
    else
        warn "Not a git checkout (or git missing) — skipping update."
    fi
else
    info "Skipping git pull (--no-pull)."
fi

# ── 2. Build (cmake + make) ──────────────────────────────────────────────────
if [[ $DO_BUILD -eq 1 ]]; then
    # Pre-flight: build tools
    if ! command -v cmake &>/dev/null; then
        error "cmake not found. Install the build dependencies (see README.md)."
        exit 1
    fi
    if ! command -v c++ &>/dev/null && ! command -v g++ &>/dev/null; then
        error "No C++ compiler found (g++). Install the build dependencies (see README.md)."
        exit 1
    fi

    info "Configuring with CMake → $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    if ! cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release; then
        error "CMake configuration failed."
        error "Most likely Qt6 is missing. See the build dependencies in README.md."
        exit 1
    fi

    info "Compiling (make -j$(nproc))..."
    cmake --build "$BUILD_DIR" -j"$(nproc)"
    success "Build finished."
else
    info "Skipping build (--no-build)."
fi

# ── Verify binary exists ─────────────────────────────────────────────────────
if [[ ! -f "$BINARY" ]]; then
    error "Compiled binary not found at: $BINARY"
    if [[ $DO_BUILD -eq 0 ]]; then
        error "You passed --no-build but no binary exists. Build it first."
    fi
    exit 1
fi

# ── Request sudo upfront (for the install steps) ─────────────────────────────
if [[ -n "$SUDO" ]]; then
    info "The install steps require root privileges."
    sudo -v || { error "Failed to obtain sudo privileges."; exit 1; }
fi

# ── 3. Install binary ────────────────────────────────────────────────────────
info "Installing binary → $BIN_DIR/$APP_NAME"
$SUDO install -Dm755 "$BINARY" "$BIN_DIR/$APP_NAME"
success "Binary installed."

# ── Install desktop entry ────────────────────────────────────────────────────
info "Installing desktop entry → $DESKTOP_DIR/"
$SUDO install -Dm644 "$PROJECT_ROOT/scripts/ezytdownloader.desktop" \
    "$DESKTOP_DIR/ezytdownloader.desktop"
success "Desktop entry installed."

# ── Install icon ─────────────────────────────────────────────────────────────
info "Installing icon → $ICON_DIR/ezytdownloader.png"
$SUDO install -Dm644 "$PROJECT_ROOT/resources/icons/app_icon.png" \
    "$ICON_DIR/ezytdownloader.png"
success "Icon installed."

# ── Update icon cache (best-effort) ──────────────────────────────────────────
if command -v gtk-update-icon-cache &>/dev/null; then
    info "Updating icon cache..."
    $SUDO gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi

# ── Update desktop database (best-effort) ────────────────────────────────────
if command -v update-desktop-database &>/dev/null; then
    info "Updating desktop database..."
    $SUDO update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi

# ── Runtime dependency reminder (best-effort) ────────────────────────────────
for dep in yt-dlp ffmpeg; do
    command -v "$dep" &>/dev/null || \
        warn "Runtime dependency '$dep' not found on PATH — install it before use (see README.md)."
done

# ── Summary ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}✅  EzYTDownloader installed successfully!${NC}"
echo ""
echo -e "  Binary:    ${CYAN}$BIN_DIR/$APP_NAME${NC}"
echo -e "  Desktop:   ${CYAN}$DESKTOP_DIR/ezytdownloader.desktop${NC}"
echo -e "  Icon:      ${CYAN}$ICON_DIR/ezytdownloader.png${NC}"
echo ""
echo -e "You can now launch ${BOLD}$APP_NAME${NC} from your application menu or terminal."
