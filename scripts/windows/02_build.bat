@echo off
rem ============================================================================
rem  Build the legs firmware for the Teensy 4.1 (Windows) - one .hex per leg
rem
rem    scripts\windows\02_build.bat            build all four legs (only what changed)
rem    scripts\windows\02_build.bat 3          build only firmware 3 (Rear Right)
rem    scripts\windows\02_build.bat clean      rebuild all four from scratch - use
rem                                            after editing prj.conf or app.overlay
rem    scripts\windows\02_build.bat 3 clean    rebuild only firmware 3 from scratch
rem
rem  Output, one file per leg (the table is scripts\legs.csv):
rem    firmware\leg1-FR-192.168.1.11.hex ... firmware\leg4-RL-192.168.1.14.hex
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"
call "%SCRIPTS%00_env.bat" || goto :fail

set "PRISTINE=auto"
set "ONLY="
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="clean" (set "PRISTINE=always") else (set "ONLY=%~1")
shift
goto :parse_args
:args_done

if defined ONLY findstr /b /c:"%ONLY%," "%LEGS_FILE%" >nul || goto :bad_leg
if not exist "%FIRMWARE_DIR%" mkdir "%FIRMWARE_DIR%"

set "COUNT=0"
for /f "usebackq eol=# tokens=1-5 delims=," %%A in ("%LEGS_FILE%") do (
    call :build_leg "%%A" "%%B" "%%C" "%%D" "%%E" || goto :fail
)

echo.
echo Build OK - %COUNT% firmware file(s) in %FIRMWARE_DIR%:
dir /b "%FIRMWARE_DIR%\*.hex"
call :maybe_pause
exit /b 0

rem ---- build one row of legs.csv: firmware code leg_id ip name ------------------
:build_leg
if defined ONLY if not "%~1"=="%ONLY%" exit /b 0
set "LEG_BUILD=%BUILD_ROOT%\leg%~1"
set "LEG_HEX=%FIRMWARE_DIR%\leg%~1-%~2-%~4.hex"
echo.
echo ==^> Firmware %~1: %~5 - LEG_ID %~3, IP %~4 - pristine: %PRISTINE%
west build -b %BOARD% -p %PRISTINE% -s "%APP_DIR%" -d "%LEG_BUILD%" -- -DLEG_ID=%~3 -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"%~4\" || exit /b 1
copy /y "%LEG_BUILD%\zephyr\zephyr.hex" "%LEG_HEX%" >nul || exit /b 1
echo     -^> %LEG_HEX%
set /a COUNT+=1
exit /b 0

:bad_leg
echo.
echo ERROR: There is no firmware "%ONLY%" in %LEGS_FILE% - use a number from 1 to 4.
goto :fail

:fail
echo.
echo BUILD FAILED - look for the first "error" line above.
call :maybe_pause
exit /b 1

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof
