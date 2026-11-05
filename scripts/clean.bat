@echo off
setlocal
if /i "%~1"=="/verify" goto verification
if /i "%~1"=="/purge" goto purge
if /i "%~1"=="/?" (
  echo Usage: clean.bat [ /verify [ /apply ] ^| /purge [ /apply ] ]
  echo Removes allowlisted native compiler intermediates and preserves deliverables.
  echo /verify previews registered test outputs; /verify /apply removes them.
  echo /purge previews recognized and reviewed historical output; /purge /apply removes it.
  exit /b 0
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean.ps1"
exit /b %ERRORLEVEL%
:verification
call "%~dp0clean_verification.bat" "%~2"
exit /b %ERRORLEVEL%
:purge
if not "%~3"=="" goto purge_usage
if /i "%~2"=="/apply" goto purge_apply
if not "%~2"=="" goto purge_usage
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0purge_artifacts.ps1"
exit /b %ERRORLEVEL%
:purge_apply
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0purge_artifacts.ps1" -Apply
exit /b %ERRORLEVEL%
:purge_usage
echo Usage: clean.bat /purge [/apply]
exit /b 2
