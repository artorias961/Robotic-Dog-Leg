# Quadruped leg firmware (Teensy 4.1 + Zephyr)

C++20 implementation of the "Per-leg firmware on Teensy 4.1" diagram.
Protocol, rates and timeouts follow `Conventions.md`; the wire format is
byte-identical to the master's `protocol.hpp`.

## Diagram box → code map

| Diagram box                                           | File                       |
|-------------------------------------------------------|----------------------------|
| Net RX thread (magic+CRC, seq/gap, dispatch, t_hBeat)  | `src/rx_thread.cpp`        |
| Control thread (1 kHz tick, comms fresh?, per-tick checks, FlexPWM) | `src/control_thread.cpp` |
| Telemetry TX thread (2 ms, seq++ · timestamp · CRC16)  | `src/tx_thread.cpp`        |
| Supervisor (I²t, tracking, loss trend, WDOG deadman)   | `src/supervisor_thread.cpp`|
| setpoint slot · command k_msgq · snapshot · health flag · counters | `include/shared.hpp` + `src/shared.cpp` |
| packet format, magic 0xC0DE, CRC16                     | `include/protocol.hpp`     |
| ADC (analog sensors), FlexPWM ×3, hardware WDOG        | `src/hal.cpp` + `app.overlay` |
| timeouts, calibration placeholders, servo mapping, LEG_ID/IP check | `include/config.hpp` |

Thread priorities (Zephyr: lower = higher): control 2, RX 5, TX 7,
supervisor 9. ISRs only capture + signal (the 1 kHz k_timer gives a
semaphore; Ethernet ISRs belong to the ENET driver).

## Safety behavior (matches the diagrams exactly)

- Boot-safe: PWM disabled in `main()` before anything else; stays disabled
  until the master's acked ENABLE arrives. Boot does not wait for the
  Ethernet link (the static IP is assigned regardless).
- Motionless arming: on ENABLE, targets start at the measured joint pose.
- Setpoint gap > 50 ms while armed: hold last targets (one stale tick is
  cheaper than a jerk); heartbeat gap > 500 ms: disarm (failsafe) and latch
  a CommsLost fault.
- Per-tick (cheap): range clamp, hard overcurrent -> cut PWM, stay cut.
- Supervisor (slow, clamped to 10–50 Hz): I²t window, tracking error that
  persists 0.5 s, loss trend, stack margins of its own four threads (about
  once a second), and the deadman — it feeds the 1 s hardware WDOG only if
  the control thread set its health flag since the last pass. Hung control
  loop -> WDOG starves -> hardware reset.
- Faults stay latched until the operator acknowledges them (CLEAR_FAULTS,
  honored only while disarmed). ENABLE is refused while faults are latched.

## Receive rules (why the leg never goes deaf)

- Setpoints and heartbeats: newest wins; older packets are stale.
- Commands are never dropped as stale (a retransmit keeps its original
  sequence number). They are de-duplicated by command sequence, ACKed once
  queued, and duplicates are re-ACKed.
- If the master's uptime (header `t_ms`) jumps backwards by more than 1 s,
  the master restarted and sequence tracking resets.

## Build (one firmware, four flavors)

`02_build` (`scripts/windows/02_build.bat` or `scripts/linux/02_build.sh`)
builds all four legs from `scripts/legs.csv` and copies one `.hex` per leg
into `firmware/`:

| Firmware | Position    | `LEG_ID` | IP           |
|----------|-------------|----------|--------------|
| 1        | Front Right | 0        | 192.168.1.11 |
| 2        | Front Left  | 1        | 192.168.1.12 |
| 3        | Rear Right  | 2        | 192.168.1.13 |
| 4        | Rear Left   | 3        | 192.168.1.14 |

The master is 192.168.1.1. `LEG_ID` starts at 0 because it is the leg's index
in the code and in every packet; "firmware 1–4" labels the same four builds.
`include/config.hpp` fails the build if a leg's IP isn't
192.168.1.(11 + `LEG_ID`) or isn't on the master's subnet. Flash a leg with
`04_flash` (it asks which one); see `scripts/README.md`.

## Honest status / verification

- **Builds with Zephyr 4.4** (SDK 1.0.1): `02_build` produces all
  four firmware files with no compiler warnings, and each image contains only
  its own IP (plus the master's). It also builds with the example pins switched
  on (`LEG_PINS_WIRED 1`). It has **not run on a Teensy yet**: boot, Ethernet,
  the USB console, the watchdog and loop timing are unproven on hardware.
- The receive and arming logic was verified against the real master over
  loopback (normal boot, master restart, leg reboot, cable pull, leg that
  never arms) using a host port of `rx_thread.cpp` and the command handling.
- Checked against Zephyr's source: the teensy41 board enables the ENET MAC and
  the DP83825 PHY; the i.MX WDOG driver needs ≥ 500 ms (the overlay enables
  `wdog1`, the code uses 1 s).
- The sensor/servo pins in `app.overlay` are switched off (`LEG_PINS_WIRED 0`)
  until the wiring exists. Setting it to 1 enables the example wiring written
  there (sensors on A0–A5, servos on pins 4/5/6); edit the pins first if the
  real wiring differs. While off, the HAL builds and runs with stubbed sensors
  so the threads + protocol can be bench-tested before the frame is built.
- Week-one milestone (from the architecture review): prove Zephyr Ethernet
  on the teensy41 board with a UDP echo before building anything else on it.
- Calibration values in `config.hpp` are placeholders; the real ones live in
  the master's `robot_config.yaml` and will be pushed at ENABLE (SET_PARAM
  command is the one protocol piece deliberately left for that work).
