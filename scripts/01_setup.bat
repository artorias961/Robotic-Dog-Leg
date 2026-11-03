@echo off
rem ============================================================================
rem  One-time setup for the legs firmware scripts (Windows)
rem
rem    scripts\01_setup.bat [zephyr-workspace]        default: C:\zephyr
rem
rem  - remembers where your Zephyr workspace is (made by
rem    tools\zephyr_west\installer_west.bat) for 02_build.bat and 04_flash.bat
rem  - sets ZEPHYR_BASE for new terminals, so west and IDE for Zephyr find it
rem  - checks west, the Zephyr SDK, pyserial and teensy_loader_cli, and can
rem    build teensy_loader_cli for you if it is missing
rem
rem  Safe to run again at any time.
rem ============================================================================
setlocal EnableExtensions
set "SCRIPT_NAME=%~nx0"
for %%I in ("%~dp0.") do set "SCRIPT_DIR=%%~fI"
for %%I in ("%SCRIPT_DIR%\..") do set "APP_DIR=%%~fI"
for %%I in ("%APP_DIR%\..\..") do set "REPO_DIR=%%~fI"

rem ---- 1. Project folder --------------------------------------------------------
echo.
echo ==^> 1/5 Project folder: %APP_DIR%
if not "%APP_DIR: =%"=="%APP_DIR%" (
    echo ERROR: Zephyr cannot build from a folder path that contains spaces.
    echo        Move the project, for example to C:\dev\Robotic-Dog, then run this again.
    goto :fail
)
echo "%APP_DIR%" | findstr /i "OneDrive Dropbox" >nul && echo WARNING: The project is in a synced folder, so build output gets synced too.

rem ---- 2. Zephyr workspace ------------------------------------------------------
echo.
echo ==^> 2/5 Zephyr workspace
set "DEFAULT_WS=C:\zephyr"
if exist "%SCRIPT_DIR%\.workspace" set /p DEFAULT_WS=<"%SCRIPT_DIR%\.workspace"
set "WS=%~1"
if not defined WS set /p "WS=Zephyr workspace folder [%DEFAULT_WS%]: "
if not defined WS set "WS=%DEFAULT_WS%"
for %%I in ("%WS%") do set "WS=%%~fI"
if "%WS:~-1%"=="\" set "WS=%WS:~0,-1%"

set "BAD_WS="
if not exist "%WS%\.west\" set "BAD_WS=1"
if not exist "%WS%\zephyr\VERSION" set "BAD_WS=1"
if not exist "%WS%\.venv\Scripts\activate.bat" set "BAD_WS=1"
if defined BAD_WS (
    echo ERROR: %WS% is not a complete Zephyr workspace - .west, zephyr or .venv is missing.
    echo        Create it with %REPO_DIR%\tools\zephyr_west\installer_west.bat
    goto :fail
)
> "%SCRIPT_DIR%\.workspace" echo %WS%
setx ZEPHYR_BASE "%WS%\zephyr" >nul
echo    Using %WS%
echo    Saved for 02_build.bat and 04_flash.bat.
echo    ZEPHYR_BASE=%WS%\zephyr is set for new terminals.

rem ---- 3. west and Zephyr ---------------------------------------------------------
echo.
echo ==^> 3/5 west and Zephyr
call "%SCRIPT_DIR%\00_env.bat" || goto :fail
west --version || goto :fail
python -c "import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)" || (
    echo ERROR: The Zephyr environment uses Python older than 3.12; Zephyr v4.4.0 needs 3.12+.
    echo        Run tools\zephyr_west\installer_west.bat again - it rebuilds the environment.
    goto :fail
)
for /f "delims=" %%V in ('python --version 2^>^&1') do echo    %%V
set "ZVER=unknown"
for /f "delims=" %%V in ('git -C "%ZEPHYR_BASE%" describe --tags 2^>nul') do set "ZVER=%%V"
echo    Zephyr %ZVER% in %ZEPHYR_BASE%

rem ---- 4. Zephyr SDK and pyserial ---------------------------------------------------
echo.
echo ==^> 4/5 Zephyr SDK and pyserial
set "SDK_DIR="
for /d %%D in ("%USERPROFILE%\zephyr-sdk-*") do set "SDK_DIR=%%~fD"
if defined SDK_DIR (
    echo    Zephyr SDK: %SDK_DIR%
) else (
    echo WARNING: No Zephyr SDK in %USERPROFILE% - run tools\zephyr_west\installer_west.bat again.
)
python -c "import serial" >nul 2>nul || (
    echo    Installing pyserial ...
    python -m pip install --quiet pyserial || goto :fail
)
echo    pyserial: OK

rem ---- 5. teensy_loader_cli -----------------------------------------------------------
echo.
echo ==^> 5/5 teensy_loader_cli - the tool that flashes the Teensy
if defined LOADER (
    echo    Found: %LOADER%
    goto :done
)
echo    Not found.
choice /c YN /m "   Build it now from PJRC's source code"
if errorlevel 2 goto :no_loader
call :build_loader || goto :fail
goto :done

:no_loader
echo    Skipped. 04_flash.bat needs it - or flash by hand with PJRC's Teensy Loader app.

:done
echo.
echo ============================================================================
echo  Setup complete. Next:
echo     scripts\02_build.bat         build what changed
echo     scripts\03_clean_build.bat   clean full rebuild
echo     scripts\04_flash.bat         build, flash, then open a serial monitor
echo ============================================================================
call :maybe_pause
exit /b 0


rem ============================================================================
rem  Subroutines
rem ============================================================================

:build_loader
rem Compiles PJRC's teensy_loader_cli into scripts\bin with MinGW gcc
set "GCC="
if exist "%REPO_DIR%\tools\mingw64\bin\gcc.exe" set "GCC=%REPO_DIR%\tools\mingw64\bin\gcc.exe"
if not defined GCC for %%F in (gcc.exe) do if not "%%~$PATH:F"=="" set "GCC=%%~$PATH:F"
if not defined GCC (
    echo ERROR: No gcc found. Put MinGW-w64 in the repo's tools\mingw64 or on PATH.
    exit /b 1
)
where git >nul 2>nul || (
    echo ERROR: git not found.
    exit /b 1
)
for %%I in ("%GCC%") do set "PATH=%%~dpI;%PATH%"
set "SRC=%TEMP%\teensy_loader_cli_src"
if exist "%SRC%" rmdir /s /q "%SRC%"
git clone --depth 1 https://github.com/PaulStoffregen/teensy_loader_cli "%SRC%" || exit /b 1
if not exist "%SCRIPT_DIR%\bin" mkdir "%SCRIPT_DIR%\bin"
echo    Compiling with %GCC%
"%GCC%" -O2 -Wall -s -DUSE_WIN32 -o "%SCRIPT_DIR%\bin\teensy_loader_cli.exe" "%SRC%\teensy_loader_cli.c" -lhid -lsetupapi -lwinmm || exit /b 1
set "LOADER=%SCRIPT_DIR%\bin\teensy_loader_cli.exe"
echo    Built: %LOADER%
exit /b 0

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof

:fail
echo.
echo SETUP FAILED - see the messages above. Fix the problem and run this again.
call :maybe_pause
exit /b 1
