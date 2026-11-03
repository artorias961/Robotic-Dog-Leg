#!/usr/bin/env bash
# =============================================================================
#  Clean full build of the legs firmware for the Teensy 4.1 (Linux)
#
#    bash scripts/03_clean_build.sh
#
#  Deletes the old build output and rebuilds everything from scratch. Use it
#  after editing prj.conf or app.overlay, or when a normal build misbehaves.
#  For everyday builds use 02_build.sh - it only rebuilds what changed.
#
#  Output: build/zephyr/zephyr.hex
# =============================================================================
set -euo pipefail
exec bash "$(dirname "${BASH_SOURCE[0]}")/02_build.sh" clean
