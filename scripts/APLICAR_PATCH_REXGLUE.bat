@echo off
setlocal
cd /d "%~dp0"

if "%~1"=="" (
    echo.
    echo Uso:
    echo   APLICAR_PATCH_REXGLUE.bat "C:\caminho\para\rexglue-sdk"
    echo.
    pause
    exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0APLICAR_PATCH_REXGLUE.ps1" -RexGlueSource "%~1"

echo.
pause
