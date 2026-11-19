@echo off
rem ============================================================================
rem  Flash one leg's firmware to a Teensy 4.1 (Windows)
rem
rem    scripts\windows\04_flash.bat        asks which leg this Teensy is
rem    scripts\windows\04_flash.bat 3      flashes firmware 3 (Rear Right)
rem
rem  1. builds that leg (only what changed), so a stale file is never flashed
rem  2. waits for you to press the Teensy's button, then flashes it
rem  3. opens a serial monitor - the firmware's first log line names the leg
rem     and its IP address
rem
rem  The Teensy is flashed through its bootloader button, not a COM port;
rem  the COM port is where the firmware's log output shows up afterwards.
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"
call "%SCRIPTS%00_env.bat" || goto :fail

rem ---- Which leg? ------------------------------------------------------------
set "LEG=%~1"
if defined LEG goto :have_leg
echo.
echo Which leg is this Teensy?
for /f "usebackq eol=# tokens=1-5 delims=," %%A in ("%LEGS_FILE%") do echo   %%A = %%E - %%B, %%D
set /p "LEG=Firmware number: "
:have_leg

set "CODE="
for /f "usebackq eol=# tokens=1-5 delims=," %%A in ("%LEGS_FILE%") do if "%%A"=="%LEG%" (
    set "CODE=%%B"
    set "LEG_ID=%%C"
    set "IP=%%D"
    set "NAME=%%E"
)
if not defined CODE (
    echo ERROR: There is no firmware "%LEG%" in %LEGS_FILE% - use a number from 1 to 4.
    goto :fail
)
set "HEX_FILE=%FIRMWARE_DIR%\leg%LEG%-%CODE%-%IP%.hex"

rem ---- 1. Build ---------------------------------------------------------------
call "%SCRIPT_DIR%\02_build.bat" %LEG% || goto :fail

rem ---- 2. Flash ---------------------------------------------------------------
if not defined LOADER (
    echo.
    echo teensy_loader_cli was not found. Run scripts\windows\01_setup.bat - it can build it for you.
    echo Or flash by hand with PJRC's Teensy Loader app: open
    echo   %HEX_FILE%
    echo and press the Teensy's button.
    goto :fail
)
echo.
echo ==^> Flashing firmware %LEG% - %NAME%, %IP%
echo     %HEX_FILE%
echo     Press the button on the Teensy now (the red LED blinks).
echo     Waiting ... press Ctrl+C to cancel.
"%LOADER%" --mcu=%TEENSY_MCU% -w -v "%HEX_FILE%" || goto :fail
echo.
echo Flashed OK. In the serial monitor, the first line should read:
echo     quadruped leg %CODE% (id %LEG_ID%, ip %IP%)
echo Label this Teensy "%LEG% - %NAME%".

rem ---- 3. Serial monitor --------------------------------------------------------
python -c "import serial" >nul 2>nul || python -m pip install --quiet pyserial
python "%SHARED_DIR%\05_serial_monitor.py" --wait 8
call :maybe_pause
exit /b 0

:fail
echo.
echo FLASH FAILED - see the messages above.
call :maybe_pause
exit /b 1

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof
