#!/usr/bin/env bash
# =============================================================================
#  One-time setup for the legs firmware scripts (Linux)
#
#    bash scripts/01_setup.sh [zephyr-workspace]        default: ~/zephyr
#
#  - remembers where your Zephyr workspace is (made by
#    tools/zephyr_west/installer_west.sh) for 02_build.sh and 04_flash.sh
#  - adds ZEPHYR_BASE to ~/.bashrc, so west and IDE for Zephyr find it
#  - checks west, the Zephyr SDK, pyserial, teensy_loader_cli and USB access
#
#  Safe to run again at any time.
# =============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="$(cd "$APP_DIR/../.." && pwd)"

step() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33mWARNING: %s\033[0m\n' "$*"; }
fail() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# ---- 1. Project folder ----------------------------------------------------------
step "1/5 Project folder: $APP_DIR"
if [[ "$APP_DIR" == *" "* ]]; then
    fail "Zephyr cannot build from a folder path that contains spaces. Move the project, then run this again."
fi
case "$APP_DIR" in
    *OneDrive*|*Dropbox*) warn "The project is in a synced folder, so build output gets synced too." ;;
esac

# ---- 2. Zephyr workspace ------------------------------------------------------------
step "2/5 Zephyr workspace"
DEFAULT_WS="$HOME/zephyr"
if [[ -f "$SCRIPT_DIR/.workspace" ]]; then
    DEFAULT_WS="$(head -n 1 "$SCRIPT_DIR/.workspace" | tr -d '\r')"
fi
WS="${1:-}"
if [[ -z "$WS" ]]; then
    read -r -p "Zephyr workspace folder [$DEFAULT_WS]: " WS || true
fi
WS="${WS:-$DEFAULT_WS}"
WS="${WS/#\~/$HOME}"
WS="$(realpath -m "$WS")"
for need in .west zephyr/VERSION .venv/bin/activate; do
    if [[ ! -e "$WS/$need" ]]; then
        fail "$WS is not a complete Zephyr workspace ($need is missing).
       Create it with:  bash $REPO_DIR/tools/zephyr_west/installer_west.sh"
    fi
done
echo "$WS" > "$SCRIPT_DIR/.workspace"
LINE="export ZEPHYR_BASE=\"$WS/zephyr\""
if grep -qxF "$LINE" "$HOME/.bashrc" 2>/dev/null; then
    echo "   ZEPHYR_BASE is already set in ~/.bashrc"
else
    printf '\n# Zephyr workspace (added by Robotic Dog scripts/01_setup.sh)\n%s\n' "$LINE" >> "$HOME/.bashrc"
    echo "   Added ZEPHYR_BASE to ~/.bashrc (new terminals pick it up)"
fi
echo "   Using $WS - saved for 02_build.sh and 04_flash.sh"

# ---- 3. west and Zephyr -----------------------------------------------------------------
step "3/5 west and Zephyr"
# shellcheck source=00_env.sh
source "$SCRIPT_DIR/00_env.sh"
west --version
if ! python -c 'import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)'; then
    fail "The Zephyr environment uses $(python --version 2>&1); Zephyr v4.4.0 needs Python 3.12+.
       Run tools/zephyr_west/installer_west.sh again - it rebuilds the environment."
fi
echo "   $(python --version 2>&1)"
echo "   Zephyr $(git -C "$ZEPHYR_BASE" describe --tags 2>/dev/null || echo unknown) in $ZEPHYR_BASE"

# ---- 4. Zephyr SDK and pyserial -------------------------------------------------------------
step "4/5 Zephyr SDK and pyserial"
SDK_DIR=""
for d in "$HOME"/zephyr-sdk-*; do
    if [[ -d "$d" ]]; then
        SDK_DIR="$d"
    fi
done
if [[ -n "$SDK_DIR" ]]; then
    echo "   Zephyr SDK: $SDK_DIR"
else
    warn "No Zephyr SDK in $HOME - run tools/zephyr_west/installer_west.sh again."
fi
if ! python -c 'import serial' 2>/dev/null; then
    echo "   Installing pyserial ..."
    python -m pip install --quiet pyserial
fi
echo "   pyserial: OK"

# ---- 5. Teensy flashing tool and USB access -----------------------------------------------------
step "5/5 teensy_loader_cli and USB access"
if [[ -n "$LOADER" ]]; then
    echo "   teensy_loader_cli: $LOADER"
else
    warn "teensy_loader_cli not found. Install it with:  sudo apt install teensy-loader-cli"
fi
if [[ -f /etc/udev/rules.d/00-teensy.rules ]]; then
    echo "   Teensy udev rules: OK"
else
    warn "PJRC's udev rules are missing, so flashing needs sudo. Run tools/zephyr_west/installer_west.sh"
fi
# The firmware's serial port (/dev/ttyACM*) belongs to the 'dialout' group
if id -nG "$USER" | grep -qw dialout; then
    echo "   Serial port access (dialout group): OK"
else
    warn "You are not in the 'dialout' group, so the serial monitor cannot open /dev/ttyACM*."
    ANSWER=""
    read -r -p "   Add $USER to 'dialout' now? Needs sudo; then log out and back in. [Y/n] " ANSWER || true
    if [[ ! "${ANSWER:-Y}" =~ ^[Nn] ]]; then
        sudo usermod -aG dialout "$USER"
        echo "   Added. Log out and back in for it to take effect."
    fi
fi

cat <<DONE

=============================================================================
 Setup complete. Next:
    bash scripts/02_build.sh         build what changed
    bash scripts/03_clean_build.sh   clean full rebuild
    bash scripts/04_flash.sh         build, flash, then open a serial monitor
=============================================================================
DONE
