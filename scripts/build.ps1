param([ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskOutput = Join-Path $taskRoot 'build\SimpleEditorTool.exe'
if ($Configuration -eq 'Debug') { $taskOutput = Join-Path $taskRoot 'artifacts\debug\SimpleEditorTool.exe' }
foreach ($taskProcess in Get-Process) {
    if (-not $taskProcess.Path -or -not [string]::Equals($taskProcess.Path, $taskOutput, [StringComparison]::OrdinalIgnoreCase)) { continue }
    $taskPid = $taskProcess.Id
    if (-not $taskProcess.CloseMainWindow() -or -not $taskProcess.WaitForExit(5000)) {
        throw "Target PID $taskPid did not close. Check for unsaved changes before rebuilding."
    }
    Write-Host "Gracefully closed target PID $taskPid ($taskOutput)."
}
$taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$taskMsbuild = & $taskVswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $taskMsbuild) { throw 'Visual Studio 2022 MSBuild with C++ and MFC is required.' }
& $taskMsbuild (Join-Path $taskRoot 'SimpleEditorTool.sln') "/p:Configuration=$Configuration" /p:Platform=x64 /m /v:minimal
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE." }
$taskUnexpected = @()
$taskDelivery = Join-Path $taskRoot 'build'
if (Test-Path -LiteralPath $taskDelivery -PathType Container) {
    $taskUnexpected = @(Get-ChildItem -LiteralPath $taskDelivery -Force |
        Where-Object { $_.PSIsContainer -or $_.Extension.ToLowerInvariant() -notin @('.exe', '.ini') })
}
if ($taskUnexpected.Count) { throw ('build must contain only EXE/INI delivery files. Unexpected entries: ' + ($taskUnexpected.Name -join ', ')) }
Write-Host "Built $Configuration x64: $taskOutput"
