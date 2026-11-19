#!/usr/bin/env bash
# =============================================================================
#  Clean full build of the legs firmware for the Teensy 4.1 (Linux)
#
#    bash scripts/linux/03_clean_build.sh      rebuild all four legs from scratch
#    bash scripts/linux/03_clean_build.sh 3    rebuild only firmware 3 from scratch
#
#  Deletes the old build output and rebuilds everything. Use it after editing
#  prj.conf or app.overlay, or when a normal build misbehaves. For everyday
#  builds use 02_build.sh - it only rebuilds what changed.
#
#  Output: firmware/leg1-FR-192.168.1.11.hex ... (one file per leg)
# =============================================================================
set -euo pipefail
exec bash "$(dirname "${BASH_SOURCE[0]}")/02_build.sh" "$@" clean
