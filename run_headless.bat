@echo off
setlocal

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

set ROM=%2
if "%ROM%"=="" (
    echo Usage: run_headless.bat [Debug^|Release] ^<rom_path^> [frames] [log_path]
    echo Example: run_headless.bat Debug commons\games\game.z64 100 output.log
    exit /b 1
)

set FRAMES=%3
if "%FRAMES%"=="" set FRAMES=60

set LOG=%4
if "%LOG%"=="" set LOG=emu64x_headless.log

set EXE=%~dp0bin\%CONFIG%-windows-x86_64\Emu64X\Emu64X.exe

if not exist "%EXE%" (
    echo ERROR: %EXE% not found. Run build.bat first.
    exit /b 1
)

echo Running Emu64X headless...
echo   ROM: %ROM%
echo   Frames: %FRAMES%
echo   Log: %LOG%
echo   Exe: %EXE%
echo.

"%EXE%" --headless --rom "%ROM%" --frames %FRAMES% --log "%LOG%"

echo.
echo Exit code: %ERRORLEVEL%
if exist "%LOG%" (
    echo Log output:
    echo ----------------------------------------
    type "%LOG%"
    echo ----------------------------------------
)
