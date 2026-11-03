#!/usr/bin/env bash
# =============================================================================
#  Flash the legs firmware to the Teensy 4.1 (Linux)
#
#    bash scripts/04_flash.sh
#
#  1. builds the firmware (only what changed)
#  2. waits for you to press the Teensy's button, then flashes it
#  3. lists the serial ports and opens a serial monitor on the one you pick
#
#  The Teensy is flashed through its bootloader button, not a serial port;
#  the serial port is where the firmware's log output shows up afterwards.
# =============================================================================
set -euo pipefail
# shellcheck source=00_env.sh
source "$(dirname "${BASH_SOURCE[0]}")/00_env.sh"

# ---- 1. Build -----------------------------------------------------------------
bash "$SCRIPT_DIR/02_build.sh"

# ---- 2. Flash -----------------------------------------------------------------
if [[ -z "$LOADER" ]]; then
    fail "teensy_loader_cli not found. Install it with:  sudo apt install teensy-loader-cli
       Or flash by hand with PJRC's Teensy Loader app: open $HEX_FILE and press the button."
fi
step "Flashing $HEX_FILE"
echo "    Press the button on the Teensy now (the red LED blinks)."
echo "    Waiting ... press Ctrl+C to cancel."
"$LOADER" --mcu="$TEENSY_MCU" -w -v "$HEX_FILE" || fail "Flashing failed - see the messages above."
printf '\nFlashed OK - the orange LED should start blinking.\n'

# ---- 3. Serial monitor --------------------------------------------------------------
if ! python -c 'import serial' 2>/dev/null; then
    python -m pip install --quiet pyserial
fi
python "$SCRIPT_DIR/05_serial_monitor.py" --wait 8
