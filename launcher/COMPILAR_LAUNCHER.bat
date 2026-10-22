@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\BUILD_LAUNCHER.ps1"
pause
