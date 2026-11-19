#!/usr/bin/env bash
# =============================================================================
#  Flash one leg's firmware to a Teensy 4.1 (Linux)
#
#    bash scripts/linux/04_flash.sh        asks which leg this Teensy is
#    bash scripts/linux/04_flash.sh 3      flashes firmware 3 (Rear Right)
#
#  1. builds that leg (only what changed), so a stale file is never flashed
#  2. waits for you to press the Teensy's button, then flashes it
#  3. opens a serial monitor - the firmware's first log line names the leg
#     and its IP address
#
#  The Teensy is flashed through its bootloader button, not a serial port;
#  the serial port is where the firmware's log output shows up afterwards.
# =============================================================================
set -euo pipefail
# shellcheck source=00_env.sh
source "$(dirname "${BASH_SOURCE[0]}")/00_env.sh"

# ---- Which leg? ------------------------------------------------------------------
LEG="${1:-}"
if [[ -z "$LEG" ]]; then
    printf '\nWhich leg is this Teensy?\n'
    while IFS=, read -r num code leg_id ip name; do
        if [[ -z "$num" || "$num" == \#* ]]; then continue; fi
        echo "  $num = ${name%$'\r'} - $code, $ip"
    done < "$LEGS_FILE"
    read -r -p "Firmware number: " LEG
fi
[[ "$LEG" =~ ^[0-9]+$ ]] || fail "\"$LEG\" is not a firmware number - use 1 to 4."
row="$(grep -E "^${LEG}," "$LEGS_FILE" | head -n 1 | tr -d '\r' || true)"
[[ -n "$row" ]] || fail "There is no firmware \"$LEG\" in $LEGS_FILE - use a number from 1 to 4."
IFS=, read -r num code leg_id ip name <<< "$row"
HEX_FILE="$FIRMWARE_DIR/leg$num-$code-$ip.hex"

# ---- 1. Build -----------------------------------------------------------------
bash "$SCRIPT_DIR/02_build.sh" "$LEG"

# ---- 2. Flash -----------------------------------------------------------------
if [[ -z "$LOADER" ]]; then
    fail "teensy_loader_cli not found. Install it with:  sudo apt install teensy-loader-cli
       Or flash by hand with PJRC's Teensy Loader app: open $HEX_FILE and press the button."
fi
step "Flashing firmware $num - $name, $ip"
echo "    $HEX_FILE"
echo "    Press the button on the Teensy now (the red LED blinks)."
echo "    Waiting ... press Ctrl+C to cancel."
"$LOADER" --mcu="$TEENSY_MCU" -w -v "$HEX_FILE" || fail "Flashing failed - see the messages above."
printf '\nFlashed OK. In the serial monitor, the first line should read:\n'
printf '    quadruped leg %s (id %s, ip %s)\n' "$code" "$leg_id" "$ip"
printf 'Label this Teensy "%s - %s".\n' "$num" "$name"

# ---- 3. Serial monitor --------------------------------------------------------------
if ! python -c 'import serial' 2>/dev/null; then
    python -m pip install --quiet pyserial
fi
python "$SHARED_DIR/05_serial_monitor.py" --wait 8
