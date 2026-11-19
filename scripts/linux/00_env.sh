# shellcheck shell=bash
# =============================================================================
#  Shared by 01_setup.sh, 02_build.sh, 03_clean_build.sh and 04_flash.sh -
#  source it, don't run it. Finds the Zephyr workspace (saved by 01_setup.sh),
#  activates its Python environment, sets ZEPHYR_BASE, and looks for
#  teensy_loader_cli.
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# scripts/ itself holds what Linux and Windows share: legs.csv, 05_serial_monitor.py
SHARED_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_DIR="$(cd "$SHARED_DIR/.." && pwd)"
REPO_DIR="$(cd "$APP_DIR/../.." && pwd)"
BOARD="teensy41"
TEENSY_MCU="TEENSY41"
# One row per leg: firmware number, code, LEG_ID, IP, name
LEGS_FILE="$SHARED_DIR/legs.csv"
# Each leg builds in build/legN; its .hex is copied to firmware/
BUILD_ROOT="$APP_DIR/build"
FIRMWARE_DIR="$APP_DIR/firmware"

step() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33mWARNING: %s\033[0m\n' "$*"; }
fail() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

if [[ "$APP_DIR" == *" "* ]]; then
    fail "Zephyr cannot build from a folder path that contains spaces: $APP_DIR"
fi
[[ -f "$LEGS_FILE" ]] || fail "The leg table is missing: $LEGS_FILE"

# ---- Zephyr workspace: saved by 01_setup.sh, else ZEPHYR_BASE, else ~/zephyr ----
if [[ -f "$SCRIPT_DIR/.workspace" ]]; then
    ZEPHYR_WS="$(head -n 1 "$SCRIPT_DIR/.workspace" | tr -d '\r')"
elif [[ -n "${ZEPHYR_BASE:-}" ]]; then
    ZEPHYR_WS="$(dirname "$ZEPHYR_BASE")"
else
    ZEPHYR_WS="$HOME/zephyr"
fi
if [[ ! -f "$ZEPHYR_WS/.venv/bin/activate" ]]; then
    fail "No Zephyr workspace found at $ZEPHYR_WS. Run scripts/linux/01_setup.sh first."
fi
export ZEPHYR_BASE="$ZEPHYR_WS/zephyr"
set +u
# shellcheck disable=SC1091
source "$ZEPHYR_WS/.venv/bin/activate"
set -u

# ---- teensy_loader_cli: TEENSY_LOADER, PATH, scripts/linux/bin ------------------
LOADER=""
if [[ -n "${TEENSY_LOADER:-}" && -x "${TEENSY_LOADER:-}" ]]; then
    LOADER="$TEENSY_LOADER"
elif command -v teensy_loader_cli >/dev/null 2>&1; then
    LOADER="$(command -v teensy_loader_cli)"
elif [[ -x "$SCRIPT_DIR/bin/teensy_loader_cli" ]]; then
    LOADER="$SCRIPT_DIR/bin/teensy_loader_cli"
fi
