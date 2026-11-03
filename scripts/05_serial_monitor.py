#!/usr/bin/env python3
"""Pick a serial port from a numbered list and open a serial monitor on it.

    python scripts/05_serial_monitor.py [--wait SECONDS] [--baud BAUD]

--wait keeps looking for the Teensy's port for up to SECONDS (right after
flashing it takes a moment to show up). Quit the monitor with Ctrl+].
"""

import argparse
import subprocess
import sys
import time

try:
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is not installed. Run:  python -m pip install pyserial")

# USB vendor IDs used to recognise a Teensy
ZEPHYR_VID = 0x2FE3  # Zephyr's default ID, used by this firmware's USB console
PJRC_VID = 0x16C0    # PJRC, a Teensy running Arduino / Teensyduino code


def teensy_label(port):
    if port.vid == ZEPHYR_VID:
        return "Teensy (Zephyr firmware)"
    if port.vid == PJRC_VID:
        return "Teensy (Teensyduino)"
    return ""


def find_ports(wait):
    """List the serial ports, waiting up to `wait` seconds for a Teensy."""
    deadline = time.monotonic() + wait
    while True:
        ports = sorted(list_ports.comports(), key=lambda p: p.device)
        if any(teensy_label(p) for p in ports) or time.monotonic() >= deadline:
            return ports
        time.sleep(0.5)


def choose(ports):
    """Show the numbered list; return a port, "refresh", or None to skip."""
    teensies = [n for n, p in enumerate(ports, 1) if teensy_label(p)]
    default = teensies[0] if len(teensies) == 1 else None

    print("\nSerial ports:")
    if not ports:
        print("  (none found - is the Teensy plugged in?)")
    for n, p in enumerate(ports, 1):
        tag = teensy_label(p)
        print(f"  {n}) {p.device:<14} {p.description}" + (f"   <-- {tag}" if tag else ""))
    print("  r) refresh the list")
    print("  0) skip - don't open a serial monitor")

    hint = f" [{default}]" if default else ""
    while True:
        try:
            answer = input(f"Choose a port number{hint}: ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            return None
        if not answer and default:
            return ports[default - 1]
        if answer == "r":
            return "refresh"
        if answer == "0":
            return None
        if answer.isdigit() and 1 <= int(answer) <= len(ports):
            return ports[int(answer) - 1]
        print("  Type one of the numbers above.")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--wait", type=float, default=0,
                        help="seconds to wait for the Teensy's port to appear")
    parser.add_argument("--baud", type=int, default=115200,
                        help="baud rate (USB serial ignores it; default 115200)")
    args = parser.parse_args()

    if args.wait:
        print(f"\nLooking for the Teensy's serial port (up to {args.wait:g} s) ...")
    ports = find_ports(args.wait)
    while True:
        port = choose(ports)
        if port != "refresh":
            break
        ports = find_ports(0)

    if port is None:
        print("No serial monitor opened.")
        return 0

    print(f"\nOpening {port.device} - quit with Ctrl+]\n", flush=True)
    try:
        return subprocess.call(
            [sys.executable, "-m", "serial.tools.miniterm", port.device, str(args.baud)])
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    sys.exit(main())
