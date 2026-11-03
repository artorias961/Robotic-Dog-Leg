@echo off
rem ============================================================================
rem  Build the legs firmware for the Teensy 4.1 (Windows)
rem
rem    scripts\02_build.bat          build what changed
rem    scripts\02_build.bat clean    rebuild everything - use after editing
rem                               prj.conf or app.overlay (same as 03_clean_build.bat)
rem
rem  Output: build\zephyr\zephyr.hex
rem ============================================================================
setlocal
set "SCRIPT_NAME=%~nx0"
set "SCRIPTS=%~dp0"
call "%SCRIPTS%00_env.bat" || goto :fail

set "PRISTINE=auto"
if /i "%~1"=="clean" set "PRISTINE=always"

echo.
echo ==^> Building %APP_DIR% for %BOARD% - pristine: %PRISTINE%
west build -b %BOARD% -p %PRISTINE% -s "%APP_DIR%" -d "%BUILD_DIR%" || goto :fail

echo.
echo Build OK: %HEX_FILE%
call :maybe_pause
exit /b 0

:fail
echo.
echo BUILD FAILED - look for the first "error" line above.
call :maybe_pause
exit /b 1

:maybe_pause
rem Keep the window open when the script was started by double-clicking it
echo %cmdcmdline% | findstr /i /c:"%SCRIPT_NAME%" >nul && pause
goto :eof
