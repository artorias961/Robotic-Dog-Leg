# Scripts

Run these from the `submodule/legs` folder, in number order. The first time,
start at **01**; after that you mostly need **02** and **04**.

| # | Windows | Linux | What it does |
| --- | --- | --- | --- |
| 00 | `00_env.bat` | `00_env.sh` | Helper used by the other scripts. **Don't run it yourself.** |
| 01 | `scripts\01_setup.bat` | `bash scripts/01_setup.sh` | **Once:** tells the scripts where Zephyr is and checks your tools |
| 02 | `scripts\02_build.bat` | `bash scripts/02_build.sh` | Builds the firmware, recompiling only what changed |
| 03 | `scripts\03_clean_build.bat` | `bash scripts/03_clean_build.sh` | Deletes the old build and rebuilds everything. Use after editing `prj.conf` or `app.overlay` |
| 04 | `scripts\04_flash.bat` | `bash scripts/04_flash.sh` | Builds, flashes, then opens a serial monitor |
| 05 | `python scripts\05_serial_monitor.py` | `python scripts/05_serial_monitor.py` | Shows the Teensy's log output |

## Flashing

1. Run `04_flash`.
2. When it says **Press the button**, press the button on the Teensy.
3. Pick the port marked `Teensy (Zephyr firmware)` (press Enter).
4. Quit the monitor with `Ctrl+]`.

## Good to know

- The project path must not have spaces in it.
- Close the Teensy Loader app (`teensy.exe`) before running `04_flash`.
