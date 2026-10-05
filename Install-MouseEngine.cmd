@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0packaging\install.ps1" -PackageRoot "%~dp0"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" pause
exit /b %RC%
