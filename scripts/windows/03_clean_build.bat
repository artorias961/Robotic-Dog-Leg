@echo off
rem ============================================================================
rem  Clean full build of the legs firmware for the Teensy 4.1 (Windows)
rem
rem    scripts\windows\03_clean_build.bat      rebuild all four legs from scratch
rem    scripts\windows\03_clean_build.bat 3    rebuild only firmware 3 from scratch
rem
rem  Deletes the old build output and rebuilds everything. Use it after editing
rem  prj.conf or app.overlay, or when a normal build misbehaves. For everyday
rem  builds use 02_build.bat - it only rebuilds what changed.
rem
rem  Output: firmware\leg1-FR-192.168.1.11.hex ... (one file per leg)
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"
call "%SCRIPTS%02_build.bat" %1 clean
set "RESULT=%errorlevel%"
call :maybe_pause
exit /b %RESULT%

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof
