#!/usr/bin/env bash
# =============================================================================
#  Build the legs firmware for the Teensy 4.1 (Linux) - one .hex per leg
#
#    bash scripts/linux/02_build.sh            build all four legs (only what changed)
#    bash scripts/linux/02_build.sh 3          build only firmware 3 (Rear Right)
#    bash scripts/linux/02_build.sh clean      rebuild all four from scratch - use
#                                              after editing prj.conf or app.overlay
#    bash scripts/linux/02_build.sh 3 clean    rebuild only firmware 3 from scratch
#
#  Output, one file per leg (the table is scripts/legs.csv):
#    firmware/leg1-FR-192.168.1.11.hex ... firmware/leg4-RL-192.168.1.14.hex
# =============================================================================
set -euo pipefail
# shellcheck source=00_env.sh
source "$(dirname "${BASH_SOURCE[0]}")/00_env.sh"

PRISTINE="auto"
ONLY=""
for arg in "$@"; do
    if [[ "$arg" == "clean" ]]; then
        PRISTINE="always"
    else
        ONLY="$arg"
    fi
done

mkdir -p "$FIRMWARE_DIR"
count=0
# Read the table on file descriptor 3 so nothing west runs can consume it.
while IFS=, read -r num code leg_id ip name <&3; do
    if [[ -z "$num" || "$num" == \#* ]]; then continue; fi
    if [[ -n "$ONLY" && "$num" != "$ONLY" ]]; then continue; fi
    name="${name%$'\r'}"
    leg_build="$BUILD_ROOT/leg$num"
    leg_hex="$FIRMWARE_DIR/leg$num-$code-$ip.hex"

    step "Firmware $num: $name - LEG_ID $leg_id, IP $ip - pristine: $PRISTINE"
    if ! west build -b "$BOARD" -p "$PRISTINE" -s "$APP_DIR" -d "$leg_build" -- \
            -DLEG_ID="$leg_id" -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\""$ip"\"; then
        fail "Build of firmware $num failed - look for the first 'error' line above."
    fi
    cp "$leg_build/zephyr/zephyr.hex" "$leg_hex"
    echo "    -> $leg_hex"
    count=$((count + 1))
done 3< "$LEGS_FILE"

if [[ $count -eq 0 ]]; then
    fail "There is no firmware \"$ONLY\" in $LEGS_FILE - use a number from 1 to 4."
fi
printf '\nBuild OK - %d firmware file(s) in %s:\n' "$count" "$FIRMWARE_DIR"
ls -1 "$FIRMWARE_DIR"/*.hex
