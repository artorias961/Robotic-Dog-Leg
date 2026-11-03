@echo off
rem ============================================================================
rem  Shared by 02_build.bat, 04_flash.bat and 01_setup.bat - not meant to be run alone.
rem  Finds the Zephyr workspace (saved by 01_setup.bat), activates its Python
rem  environment, sets ZEPHYR_BASE and looks for teensy_loader_cli.
rem ============================================================================

for %%I in ("%~dp0.") do set "SCRIPT_DIR=%%~fI"
for %%I in ("%SCRIPT_DIR%\..") do set "APP_DIR=%%~fI"
for %%I in ("%APP_DIR%\..\..") do set "REPO_DIR=%%~fI"
set "BOARD=teensy41"
set "TEENSY_MCU=TEENSY41"
set "BUILD_DIR=%APP_DIR%\build"
set "HEX_FILE=%BUILD_DIR%\zephyr\zephyr.hex"

if not "%APP_DIR: =%"=="%APP_DIR%" (
    echo ERROR: Zephyr cannot build from a folder path that contains spaces:
    echo          %APP_DIR%
    echo        Move the project, for example to C:\dev\Robotic-Dog
    exit /b 1
)

rem ---- Zephyr workspace: saved by 01_setup.bat, else ZEPHYR_BASE, else C:\zephyr
set "ZEPHYR_WS="
if exist "%SCRIPT_DIR%\.workspace" set /p ZEPHYR_WS=<"%SCRIPT_DIR%\.workspace"
if not defined ZEPHYR_WS if defined ZEPHYR_BASE for %%I in ("%ZEPHYR_BASE%\..") do set "ZEPHYR_WS=%%~fI"
if not defined ZEPHYR_WS set "ZEPHYR_WS=C:\zephyr"
if not exist "%ZEPHYR_WS%\.venv\Scripts\activate.bat" (
    echo ERROR: No Zephyr workspace found at %ZEPHYR_WS%
    echo        Run scripts\01_setup.bat first.
    exit /b 1
)
set "ZEPHYR_BASE=%ZEPHYR_WS%\zephyr"
call "%ZEPHYR_WS%\.venv\Scripts\activate.bat"

rem ---- teensy_loader_cli: TEENSY_LOADER, PATH, scripts\bin, repo tools\teensy
set "LOADER="
if defined TEENSY_LOADER if exist "%TEENSY_LOADER%" set "LOADER=%TEENSY_LOADER%"
if not defined LOADER for %%F in (teensy_loader_cli.exe) do if not "%%~$PATH:F"=="" set "LOADER=%%~$PATH:F"
if not defined LOADER if exist "%SCRIPT_DIR%\bin\teensy_loader_cli.exe" set "LOADER=%SCRIPT_DIR%\bin\teensy_loader_cli.exe"
if not defined LOADER if exist "%REPO_DIR%\tools\teensy\teensy_loader_cli.exe" set "LOADER=%REPO_DIR%\tools\teensy\teensy_loader_cli.exe"
exit /b 0
