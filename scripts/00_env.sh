# shellcheck shell=bash
# =============================================================================
#  Shared by 02_build.sh, 04_flash.sh and 01_setup.sh - source it, don't run it.
#  Finds the Zephyr workspace (saved by 01_setup.sh), activates its Python
#  environment, sets ZEPHYR_BASE and looks for teensy_loader_cli.
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="$(cd "$APP_DIR/../.." && pwd)"
BOARD="teensy41"
TEENSY_MCU="TEENSY41"
BUILD_DIR="$APP_DIR/build"
HEX_FILE="$BUILD_DIR/zephyr/zephyr.hex"

step() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33mWARNING: %s\033[0m\n' "$*"; }
fail() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

if [[ "$APP_DIR" == *" "* ]]; then
    fail "Zephyr cannot build from a folder path that contains spaces: $APP_DIR"
fi

# ---- Zephyr workspace: saved by 01_setup.sh, else ZEPHYR_BASE, else ~/zephyr ----
if [[ -f "$SCRIPT_DIR/.workspace" ]]; then
    ZEPHYR_WS="$(head -n 1 "$SCRIPT_DIR/.workspace" | tr -d '\r')"
elif [[ -n "${ZEPHYR_BASE:-}" ]]; then
    ZEPHYR_WS="$(dirname "$ZEPHYR_BASE")"
else
    ZEPHYR_WS="$HOME/zephyr"
fi
if [[ ! -f "$ZEPHYR_WS/.venv/bin/activate" ]]; then
    fail "No Zephyr workspace found at $ZEPHYR_WS. Run scripts/01_setup.sh first."
fi
export ZEPHYR_BASE="$ZEPHYR_WS/zephyr"
set +u
# shellcheck disable=SC1091
source "$ZEPHYR_WS/.venv/bin/activate"
set -u

# ---- teensy_loader_cli: TEENSY_LOADER, PATH, scripts/bin ------------------------
LOADER=""
if [[ -n "${TEENSY_LOADER:-}" && -x "${TEENSY_LOADER:-}" ]]; then
    LOADER="$TEENSY_LOADER"
elif command -v teensy_loader_cli >/dev/null 2>&1; then
    LOADER="$(command -v teensy_loader_cli)"
elif [[ -x "$SCRIPT_DIR/bin/teensy_loader_cli" ]]; then
    LOADER="$SCRIPT_DIR/bin/teensy_loader_cli"
fi
