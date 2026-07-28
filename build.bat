@echo off
setlocal

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

echo Building Emu64X (%CONFIG%)...
MSBuild.exe "%~dp0Emu64X.sln" /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /m /nologo /v:minimal

if %ERRORLEVEL% neq 0 (
    echo BUILD FAILED
    exit /b %ERRORLEVEL%
)

echo BUILD SUCCEEDED
echo Output: %~dp0bin\%CONFIG%-windows-x86_64\Emu64X\Emu64X.exe
