#!/usr/bin/env bash
# ============================================================================
# install_deps.sh — Cross-distro dependency installer for EzYTDownloader
# ============================================================================
set -euo pipefail

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

banner() {
    echo -e "${CYAN}${BOLD}"
    echo "╔══════════════════════════════════════════════════════════╗"
    echo "║        EzYTDownloader — Dependency Installer            ║"
    echo "╚══════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }

# ── Detect distribution ─────────────────────────────────────────────────────
detect_distro() {
    if [[ ! -f /etc/os-release ]]; then
        error "Cannot find /etc/os-release — unable to detect your distribution."
        exit 1
    fi

    # Source the file to get $ID (and $ID_LIKE as fallback)
    # shellcheck disable=SC1091
    source /etc/os-release

    info "Detected OS: ${PRETTY_NAME:-$ID}"
}

# ── Install packages ────────────────────────────────────────────────────────
install_packages() {
    local id="${ID,,}"           # lowercase
    local id_like="${ID_LIKE,,}" # lowercase fallback

    case "$id" in
        debian|ubuntu|linuxmint|pop|elementary|zorin|kali)
            install_apt
            ;;
        fedora|rhel|centos|rocky|alma|nobara)
            install_dnf
            ;;
        arch|manjaro|endeavouros|garuda)
            install_pacman
            ;;
        opensuse*|sles)
            install_zypper
            ;;
        *)
            # Try ID_LIKE as fallback
            case "$id_like" in
                *debian*|*ubuntu*)
                    warn "Unknown distro '$id', but ID_LIKE suggests Debian-based."
                    install_apt
                    ;;
                *fedora*|*rhel*)
                    warn "Unknown distro '$id', but ID_LIKE suggests Fedora/RHEL."
                    install_dnf
                    ;;
                *arch*)
                    warn "Unknown distro '$id', but ID_LIKE suggests Arch-based."
                    install_pacman
                    ;;
                *suse*)
                    warn "Unknown distro '$id', but ID_LIKE suggests openSUSE."
                    install_zypper
                    ;;
                *)
                    error "Unsupported distribution: $id (ID_LIKE=$id_like)"
                    error "Please install manually: python3, ffmpeg, yt-dlp"
                    exit 2
                    ;;
            esac
            ;;
    esac
}

install_apt() {
    info "Using APT (Debian/Ubuntu family)"
    if ! sudo apt update; then
        error "Failed to run 'sudo apt update'. Are you authorized to use sudo?"
        exit 3
    fi
    sudo apt install -y python3 python3-pip ffmpeg || {
        error "APT install failed."
        exit 4
    }
    # yt-dlp may not be in the main repos on older Ubuntu — try pip fallback
    if ! sudo apt install -y yt-dlp 2>/dev/null; then
        warn "yt-dlp not in APT repos, installing via pip..."
        python3 -m pip install --user --break-system-packages yt-dlp 2>/dev/null \
            || python3 -m pip install --user yt-dlp \
            || { error "Failed to install yt-dlp via pip."; exit 4; }
    fi
}

install_dnf() {
    info "Using DNF (Fedora/RHEL family)"
    sudo dnf install -y python3 ffmpeg yt-dlp || {
        error "DNF install failed. Trying with pip for yt-dlp..."
        sudo dnf install -y python3 ffmpeg || { error "DNF install failed."; exit 4; }
        python3 -m pip install --user yt-dlp || { error "pip install yt-dlp failed."; exit 4; }
    }
}

install_pacman() {
    info "Using pacman (Arch family)"
    sudo pacman -Sy --noconfirm python ffmpeg yt-dlp || {
        error "pacman install failed."
        exit 4
    }
}

install_zypper() {
    info "Using zypper (openSUSE family)"
    sudo zypper install -y python3 ffmpeg yt-dlp || {
        error "zypper install failed. Trying with pip for yt-dlp..."
        sudo zypper install -y python3 ffmpeg || { error "zypper install failed."; exit 4; }
        python3 -m pip install --user yt-dlp || { error "pip install yt-dlp failed."; exit 4; }
    }
}

# ── Verify installation ─────────────────────────────────────────────────────
verify() {
    echo ""
    info "Verifying installed tools..."
    local ok=true

    for cmd in python3 ffmpeg yt-dlp bash; do
        if command -v "$cmd" &>/dev/null; then
            success "$cmd  →  $(command -v "$cmd")"
        else
            error "$cmd  →  NOT FOUND"
            ok=false
        fi
    done

    echo ""
    if $ok; then
        echo -e "${GREEN}${BOLD}✅  All dependencies installed successfully!${NC}"
        echo -e "${CYAN}You can now restart EzYTDownloader.${NC}"
    else
        echo -e "${RED}${BOLD}❌  Some dependencies could not be installed.${NC}"
        echo -e "${YELLOW}Please install them manually and restart EzYTDownloader.${NC}"
        exit 5
    fi
}

# ── Main ─────────────────────────────────────────────────────────────────────
main() {
    banner
    detect_distro
    echo ""
    install_packages
    verify
    echo ""
    read -rp "Press Enter to close this window..."
}

main "$@"
