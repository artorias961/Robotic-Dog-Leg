# Robotic-Dog-Leg

Firmware for the robotic dog's legs. It runs on a **Teensy 4.1** and is written in **C++20** on **Zephyr RTOS v4.4.0**.

> [!IMPORTANT]
> **Zephyr cannot build from a path that contains spaces.** Clone the project somewhere like `C:\dev\Robotic-Dog` or `~/dev/Robotic-Dog`, and keep it out of OneDrive/Dropbox.

## Documentation

The step-by-step guides are in the [wiki](https://github.com/artorias961/Robotic-Dog-Leg/wiki). Follow them in order:

| Page                                                                                                                  | What it covers                                                                                  |
|-----------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| [00 ‐ Installation and Setup](https://github.com/artorias961/Robotic-Dog-Leg/wiki/00-%E2%80%90-Installation-and-Setup) | Install VS Code, Zephyr, and the ARM toolchain, then flash Zephyr's Blinky sample               |
| [01 ‐ Blink an LED](https://github.com/artorias961/Robotic-Dog-Leg/wiki/01-%E2%80%90-Blink-an-LED)                     | Build and flash this firmware with the project scripts, and read its log output over USB        |

> [!NOTE]
> The 01 page was written for the earlier blink test. `src/main.cpp` is now the leg firmware; the blink test is still in git history (commit `5d84dab`).

## Quick start

You need the Zephyr workspace from the main [Robotic-Dog](https://github.com/artorias961/Robotic-Dog) project's installer (`tools/zephyr_west/`) and **Python 3.12+**. Then, from this folder:

| Step                                  | Windows (Command Prompt)       | Linux                            |
|---------------------------------------|--------------------------------|----------------------------------|
| 1. Set up (once per computer)         | `scripts\windows\01_setup.bat` | `bash scripts/linux/01_setup.sh` |
| 2. Build all four legs                | `scripts\windows\02_build.bat` | `bash scripts/linux/02_build.sh` |
| 3. Flash one leg and open the monitor | `scripts\windows\04_flash.bat` | `bash scripts/linux/04_flash.sh` |

The Windows scripts are in `scripts/windows/` and the Linux ones in `scripts/linux/`. Both read the same leg table, `scripts/legs.csv`.

`02_build` makes one firmware file per leg, each with its own leg number and IP address built in:

| Firmware | Leg         | IP           | File                                |
|----------|-------------|--------------|-------------------------------------|
| 1        | Front Right | 192.168.1.11 | `firmware/leg1-FR-192.168.1.11.hex` |
| 2        | Front Left  | 192.168.1.12 | `firmware/leg2-FL-192.168.1.12.hex` |
| 3        | Rear Right  | 192.168.1.13 | `firmware/leg3-RR-192.168.1.13.hex` |
| 4        | Rear Left   | 192.168.1.14 | `firmware/leg4-RL-192.168.1.14.hex` |

`04_flash` asks which leg the Teensy is (or run `scripts\windows\04_flash.bat 3`). When it prints **Press the button on the Teensy now**, press the Teensy's program button. The serial monitor's first line names the leg and its IP, for example `quadruped leg FR (id 0, ip 192.168.1.11)`. Label each Teensy with its firmware number. Quit the monitor with **Ctrl+]**.

After editing `prj.conf` or `app.overlay`, do a clean full build: `scripts\windows\03_clean_build.bat` or `bash scripts/linux/03_clean_build.sh`. See [scripts/README.md](scripts/README.md) for building or flashing a single leg.

## What's inside

```
CMakeLists.txt                Connects the app to Zephyr
prj.conf                      Zephyr settings: C++20, FPU, USB console, networking
app.overlay                   USB console, hardware watchdog, sensor/servo pin template
include/                      Firmware headers (config, protocol, shared data, HAL)
src/                          Leg firmware: main.cpp plus one file per thread (see src/README.md)
scripts/windows/              Setup, build and flash scripts for Windows (.bat, run in number order)
scripts/linux/                The same scripts for Linux (.sh)
scripts/legs.csv              The leg table both use: firmware number, LEG_ID, IP
scripts/05_serial_monitor.py  The serial monitor both use (Python)
firmware/                     Build output: one .hex per leg (made by 02_build, ignored by git)
```

## Board and toolchain

| Item                | Value                                                           |
|---------------------|-----------------------------------------------------------------|
| Board               | PJRC Teensy 4.1 (NXP i.MX RT1062, ARM Cortex-M7, 600 MHz)       |
| Zephyr board target | `teensy41`                                                      |
| Zephyr              | v4.4.0                                                          |
| Toolchain           | Zephyr SDK 1.0.1, `arm-zephyr-eabi`                             |
| Language            | C++20 (`CONFIG_STD_CPP20`)                                      |
| Flashing            | Teensy program button + `teensy_loader_cli`, or PJRC's Teensy Loader |

## Building by hand

The scripts wrap one `west build` per leg. Activate the workspace environment first (`call C:\zephyr\.venv\Scripts\activate.bat` or `source ~/zephyr/.venv/bin/activate`), then from this folder, for example for firmware 2 (Front Left):

```
west build -b teensy41 -p always -d build/leg2 -- -DLEG_ID=1 -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"192.168.1.12\"
```

The leg number and IP must match (`192.168.1.(11 + LEG_ID)`); the firmware refuses to build otherwise. Press the Teensy's program button, then flash `build/leg2/zephyr/zephyr.hex` with `teensy_loader_cli` or [PJRC's Teensy Loader](https://www.pjrc.com/teensy/loader.html).
