@echo off
setlocal
cd /d "%~dp0"

if "%~1"=="" (
    set "TARGET=..\generated"
) else (
    set "TARGET=%~1"
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0AUDITAR_BCTR_GERADO.ps1" -GeneratedRoot "%TARGET%"

echo.
pause
