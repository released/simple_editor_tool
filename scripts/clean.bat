@echo off
setlocal
if /i "%~1"=="/?" (
  echo Usage: clean.bat
  echo Removes allowlisted native compiler intermediates and preserves deliverables.
  exit /b 0
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean.ps1"
exit /b %ERRORLEVEL%
