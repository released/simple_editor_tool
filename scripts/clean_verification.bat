@echo off
setlocal
if /i "%~1"=="/apply" goto apply
if not "%~1"=="" (
  echo Usage: clean_verification.bat [/apply]
  echo Default: preview registered verification outputs. /apply: remove them safely.
  if /i "%~1"=="/?" exit /b 0
  exit /b 2
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_verification.ps1"
exit /b %ERRORLEVEL%
:apply
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_verification.ps1" -Apply
exit /b %ERRORLEVEL%
