@echo off
setlocal
cd /d "%~dp0"

title JoJo HD Recompiled - One Click Build

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BUILD_JOJO.ps1"
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
    echo ============================================================
    echo   BUILD FINALIZADO COM SUCESSO
    echo ============================================================
) else (
    echo ============================================================
    echo   BUILD INTERROMPIDO - CODIGO %RC%
    echo ============================================================
)
echo.
pause
exit /b %RC%
