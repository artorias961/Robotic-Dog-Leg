@echo off
rem ============================================================================
rem  Clean full build of the legs firmware for the Teensy 4.1 (Windows)
rem
rem    scripts\03_clean_build.bat
rem
rem  Deletes the old build output and rebuilds everything from scratch. Use it
rem  after editing prj.conf or app.overlay, or when a normal build misbehaves.
rem  For everyday builds use 02_build.bat - it only rebuilds what changed.
rem
rem  Output: build\zephyr\zephyr.hex
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"

call "%SCRIPTS%02_build.bat" clean
set "RESULT=%errorlevel%"
call :maybe_pause
exit /b %RESULT%

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof
