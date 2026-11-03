@echo off
rem ============================================================================
rem  Flash the legs firmware to the Teensy 4.1 (Windows)
rem
rem    scripts\04_flash.bat
rem
rem  1. builds the firmware (only what changed)
rem  2. waits for you to press the Teensy's button, then flashes it
rem  3. lists the COM ports and opens a serial monitor on the one you pick
rem
rem  The Teensy is flashed through its bootloader button, not a COM port;
rem  the COM port is where the firmware's log output shows up afterwards.
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"
call "%SCRIPTS%00_env.bat" || goto :fail

rem ---- 1. Build ---------------------------------------------------------------
call "%SCRIPT_DIR%\02_build.bat" || goto :fail

rem ---- 2. Flash ---------------------------------------------------------------
if not defined LOADER (
    echo.
    echo teensy_loader_cli was not found. Run scripts\01_setup.bat - it can build it for you.
    echo Or flash by hand with PJRC's Teensy Loader app: open
    echo   %HEX_FILE%
    echo and press the Teensy's button.
    goto :fail
)
echo.
echo ==^> Flashing %HEX_FILE%
echo     Press the button on the Teensy now (the red LED blinks).
echo     Waiting ... press Ctrl+C to cancel.
"%LOADER%" --mcu=%TEENSY_MCU% -w -v "%HEX_FILE%" || goto :fail
echo.
echo Flashed OK - the orange LED should start blinking.

rem ---- 3. Serial monitor --------------------------------------------------------
python -c "import serial" >nul 2>nul || python -m pip install --quiet pyserial
python "%SCRIPT_DIR%\05_serial_monitor.py" --wait 8
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
