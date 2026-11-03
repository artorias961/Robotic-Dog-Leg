#!/usr/bin/env bash
# =============================================================================
#  Build the legs firmware for the Teensy 4.1 (Linux)
#
#    bash scripts/02_build.sh          build what changed
#    bash scripts/02_build.sh clean    rebuild everything - use after editing
#                                   prj.conf or app.overlay (same as 03_clean_build.sh)
#
#  Output: build/zephyr/zephyr.hex
# =============================================================================
set -euo pipefail
# shellcheck source=00_env.sh
source "$(dirname "${BASH_SOURCE[0]}")/00_env.sh"

PRISTINE="auto"
if [[ "${1:-}" == "clean" ]]; then
    PRISTINE="always"
fi

step "Building $APP_DIR for $BOARD - pristine: $PRISTINE"
if ! west build -b "$BOARD" -p "$PRISTINE" -s "$APP_DIR" -d "$BUILD_DIR"; then
    fail "Build failed - look for the first 'error' line above."
fi
printf '\nBuild OK: %s\n' "$HEX_FILE"
