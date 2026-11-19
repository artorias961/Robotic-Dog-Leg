# Scripts

The scripts are split by operating system. Both sets do the same jobs in the
same numbered order, and they share one leg table and one serial monitor:

```
scripts/
├── windows/                 Command Prompt scripts (.bat)
│   ├── 00_env.bat
│   ├── 01_setup.bat
│   ├── 02_build.bat
│   ├── 03_clean_build.bat
│   └── 04_flash.bat
├── linux/                   bash scripts (.sh)
│   ├── 00_env.sh
│   ├── 01_setup.sh
│   ├── 02_build.sh
│   ├── 03_clean_build.sh
│   └── 04_flash.sh
├── legs.csv                 the leg table both use: firmware number, LEG_ID, IP
└── 05_serial_monitor.py     the serial monitor both use (Python)
```

Run them from the `submodule/legs` folder, in number order. The first time,
start at **01**; after that you mostly need **02** and **04**.

| # | Windows (Command Prompt) | Linux | What it does |
| --- | --- | --- | --- |
| 00 | `scripts\windows\00_env.bat` | `scripts/linux/00_env.sh` | Helper used by the other scripts. **Don't run it yourself.** |
| 01 | `scripts\windows\01_setup.bat` | `bash scripts/linux/01_setup.sh` | **Once:** tells the scripts where Zephyr is and checks your tools |
| 02 | `scripts\windows\02_build.bat` | `bash scripts/linux/02_build.sh` | **Makes the four firmware files** in `firmware/`, recompiling only what changed |
| 03 | `scripts\windows\03_clean_build.bat` | `bash scripts/linux/03_clean_build.sh` | Deletes the old builds and runs 02 from scratch. Use after editing `prj.conf` or `app.overlay` |
| 04 | `scripts\windows\04_flash.bat` | `bash scripts/linux/04_flash.sh` | Asks which leg, builds it with 02, flashes it, then opens the serial monitor |
| 05 | `python scripts\05_serial_monitor.py` | `python scripts/05_serial_monitor.py` | Shows the Teensy's log output (04 opens it for you) |

## One firmware per leg

`02_build` makes the four files. For each row of `legs.csv` it runs one Zephyr
build with that row's `LEG_ID` and IP, in that leg's own folder (`build/leg1`
... `build/leg4`, so legs never share a build), then copies the result into
`firmware/`:

| File | Leg | `LEG_ID` | IP |
| --- | --- | --- | --- |
| `firmware/leg1-FR-192.168.1.11.hex` | Front Right | 0 | 192.168.1.11 |
| `firmware/leg2-FL-192.168.1.12.hex` | Front Left | 1 | 192.168.1.12 |
| `firmware/leg3-RR-192.168.1.13.hex` | Rear Right | 2 | 192.168.1.13 |
| `firmware/leg4-RL-192.168.1.14.hex` | Rear Left | 3 | 192.168.1.14 |

The Windows and Linux scripts both read this one table. The firmware refuses
to build if a leg's IP doesn't match its `LEG_ID` (it must be
192.168.1.(11 + LEG_ID)).

Add a number to build or flash just one leg, and `clean` to rebuild from
scratch:

```
scripts\windows\02_build.bat            all four legs
scripts\windows\02_build.bat 3          only firmware 3 (Rear Right)
scripts\windows\02_build.bat 3 clean    only firmware 3, from scratch
scripts\windows\04_flash.bat 3          build and flash firmware 3
```

On Linux it is the same with `bash scripts/linux/02_build.sh 3`, and so on.

## Flashing

1. Run `04_flash`, and type the leg's number if it asks.
2. When it says **Press the button**, press the button on the Teensy.
3. Pick the port marked `Teensy (Zephyr firmware)` (press Enter).
4. The first log line names the leg and its IP, for example
   `quadruped leg FR (id 0, ip 192.168.1.11)`. Label the Teensy with its
   firmware number.
5. Quit the monitor with `Ctrl+]`.

## Good to know

- The project path must not have spaces in it, and should be outside
  OneDrive/Dropbox.
- Close the Teensy Loader app (`teensy.exe`) before running `04_flash`.
- The `firmware/` and `build/` folders are build output; git ignores them.
  It also ignores what `01_setup` saves for your computer inside `windows/` or
  `linux/`: `.workspace` (where Zephyr is) and `bin/` (a teensy_loader_cli it
  built for you).
