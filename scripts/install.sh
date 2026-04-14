#!/usr/bin/env bash
# ============================================================================
# install.sh — System-wide installer for EzYTDownloader
# Installs the binary, desktop entry, icon, and supporting files.
# Must be run from the repository root (where CMakeLists.txt lives).
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
BINARY="$PROJECT_ROOT/build/$APP_NAME"

PREFIX="/usr/local"
BIN_DIR="$PREFIX/bin"
DATA_DIR="$PREFIX/share/$APP_NAME"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/256x256/apps"

# ── Banner ───────────────────────────────────────────────────────────────────
echo -e "${CYAN}${BOLD}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║          EzYTDownloader — System Installer              ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ── Pre-flight checks ───────────────────────────────────────────────────────
if [[ ! -f "$BINARY" ]]; then
    error "Compiled binary not found at: $BINARY"
    error "Please build the project first:"
    echo  "    mkdir -p build && cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

# ── Request sudo upfront ────────────────────────────────────────────────────
if [[ $EUID -ne 0 ]]; then
    info "This installer requires root privileges."
    sudo -v || { error "Failed to obtain sudo privileges."; exit 1; }
fi

# ── Install binary ──────────────────────────────────────────────────────────
info "Installing binary → $BIN_DIR/$APP_NAME"
sudo install -Dm755 "$BINARY" "$BIN_DIR/$APP_NAME"
success "Binary installed."

# ── Install Python backend ──────────────────────────────────────────────────
info "Installing Python backend → $DATA_DIR/python/"
sudo install -Dm755 "$PROJECT_ROOT/python/fetch.py" "$DATA_DIR/python/fetch.py"
success "Python backend installed."

# ── Install helper scripts ──────────────────────────────────────────────────
info "Installing helper scripts → $DATA_DIR/scripts/"
sudo install -Dm755 "$PROJECT_ROOT/scripts/install_deps.sh" "$DATA_DIR/scripts/install_deps.sh"
success "Helper scripts installed."

# ── Install desktop entry ───────────────────────────────────────────────────
info "Installing desktop entry → $DESKTOP_DIR/"
sudo install -Dm644 "$PROJECT_ROOT/scripts/ezytdownloader.desktop" \
    "$DESKTOP_DIR/ezytdownloader.desktop"
success "Desktop entry installed."

# ── Install icon ────────────────────────────────────────────────────────────
info "Installing icon → $ICON_DIR/ezytdownloader.png"
sudo install -Dm644 "$PROJECT_ROOT/resources/icons/app_icon.png" \
    "$ICON_DIR/ezytdownloader.png"
success "Icon installed."

# ── Update icon cache (best-effort) ─────────────────────────────────────────
if command -v gtk-update-icon-cache &>/dev/null; then
    info "Updating icon cache..."
    sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi

# ── Update desktop database (best-effort) ───────────────────────────────────
if command -v update-desktop-database &>/dev/null; then
    info "Updating desktop database..."
    sudo update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi

# ── Summary ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}✅  EzYTDownloader installed successfully!${NC}"
echo ""
echo -e "  Binary:    ${CYAN}$BIN_DIR/$APP_NAME${NC}"
echo -e "  Data:      ${CYAN}$DATA_DIR/${NC}"
echo -e "  Desktop:   ${CYAN}$DESKTOP_DIR/ezytdownloader.desktop${NC}"
echo -e "  Icon:      ${CYAN}$ICON_DIR/ezytdownloader.png${NC}"
echo ""
echo -e "You can now launch ${BOLD}$APP_NAME${NC} from your application menu or terminal."
