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

## Quick start

You need the Zephyr workspace from the main [Robotic-Dog](https://github.com/artorias961/Robotic-Dog) project's installer (`tools/zephyr_west/`) and **Python 3.12+**. Then, from this folder:

| Step                         | Windows (Command Prompt) | Linux                      |
|------------------------------|--------------------------|----------------------------|
| 1. Set up (once per computer) | `scripts\01_setup.bat`   | `bash scripts/01_setup.sh` |
| 2. Build                     | `scripts\02_build.bat`   | `bash scripts/02_build.sh` |
| 3. Flash and open the monitor | `scripts\04_flash.bat`   | `bash scripts/04_flash.sh` |

When `04_flash` prints **Press the button on the Teensy now**, press the Teensy's program button. The orange LED then blinks once a second, and the serial monitor shows the firmware's log every 2 seconds. Quit the monitor with **Ctrl+]**.

After editing `prj.conf` or `app.overlay`, do a clean full build: `scripts\03_clean_build.bat` or `bash scripts/03_clean_build.sh`.

## What's inside

```
CMakeLists.txt        Connects the app to Zephyr
prj.conf              Zephyr settings: C++20, LED, logging, USB console
app.overlay           Sends the console to the Teensy's USB port
src/main.cpp          The firmware
scripts/              Setup, build, flash, and serial monitor scripts (run in number order)
docs/wiki/            Source of the wiki pages
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

The scripts wrap these `west` commands. Activate the workspace environment first (`call C:\zephyr\.venv\Scripts\activate.bat` or `source ~/zephyr/.venv/bin/activate`), then from this folder:

```
west build -b teensy41 -p always
west flash
```

Press the Teensy's program button before `west flash`. Without `teensy_loader_cli`, open `build/zephyr/zephyr.hex` in [PJRC's Teensy Loader](https://www.pjrc.com/teensy/loader.html) and press the button.
