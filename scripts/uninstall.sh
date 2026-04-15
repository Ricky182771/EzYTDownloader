#!/usr/bin/env bash
# ============================================================================
# uninstall.sh — System-wide uninstaller for EzYTDownloader
# Removes the binary, desktop entry, icon, and supporting files.
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

# ── Paths (must match install.sh) ────────────────────────────────────────────
APP_NAME="EzYTDownloader"

PREFIX="/usr/local"
BIN_DIR="$PREFIX/bin"
DATA_DIR="$PREFIX/share/$APP_NAME"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/256x256/apps"

# ── Banner ───────────────────────────────────────────────────────────────────
echo -e "${CYAN}${BOLD}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║          EzYTDownloader — System Uninstaller            ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ── Confirmation ─────────────────────────────────────────────────────────────
echo -e "This will remove the following:"
echo -e "  • ${CYAN}$BIN_DIR/$APP_NAME${NC}"
echo -e "  • ${CYAN}$DATA_DIR/${NC}  (application data)"
echo -e "  • ${CYAN}$DESKTOP_DIR/ezytdownloader.desktop${NC}"
echo -e "  • ${CYAN}$ICON_DIR/ezytdownloader.png${NC}"
echo ""
read -rp "Continue with uninstall? [y/N] " answer
if [[ ! "$answer" =~ ^[Yy]$ ]]; then
    info "Uninstall cancelled."
    exit 0
fi

# ── Request sudo upfront ────────────────────────────────────────────────────
if [[ $EUID -ne 0 ]]; then
    info "This uninstaller requires root privileges."
    sudo -v || { error "Failed to obtain sudo privileges."; exit 1; }
fi

# ── Remove binary ──────────────────────────────────────────────────────────
if [[ -f "$BIN_DIR/$APP_NAME" ]]; then
    info "Removing binary..."
    sudo rm -f "$BIN_DIR/$APP_NAME"
    success "Binary removed."
else
    warn "Binary not found at $BIN_DIR/$APP_NAME (skipped)."
fi

# ── Remove data directory ──────────────────────────────────────────────────
if [[ -d "$DATA_DIR" ]]; then
    info "Removing data directory..."
    sudo rm -rf "$DATA_DIR"
    success "Data directory removed."
else
    warn "Data directory not found at $DATA_DIR (skipped)."
fi

# ── Remove desktop entry ───────────────────────────────────────────────────
if [[ -f "$DESKTOP_DIR/ezytdownloader.desktop" ]]; then
    info "Removing desktop entry..."
    sudo rm -f "$DESKTOP_DIR/ezytdownloader.desktop"
    success "Desktop entry removed."
else
    warn "Desktop entry not found (skipped)."
fi

# ── Remove icon ─────────────────────────────────────────────────────────────
if [[ -f "$ICON_DIR/ezytdownloader.png" ]]; then
    info "Removing icon..."
    sudo rm -f "$ICON_DIR/ezytdownloader.png"
    success "Icon removed."
else
    warn "Icon not found (skipped)."
fi

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
echo -e "${GREEN}${BOLD}✅  EzYTDownloader has been uninstalled.${NC}"
echo ""
