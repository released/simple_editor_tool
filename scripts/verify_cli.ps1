param([string]$Executable = (Join-Path $PSScriptRoot '..\build\SimpleEditorTool.exe'))
$ErrorActionPreference = 'Stop'
$taskProject = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskExe = [IO.Path]::GetFullPath($Executable)
$taskRoot = Join-Path $taskProject ('artifacts\tests\cli-owned-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $taskRoot)
$taskCopy = Join-Path $taskRoot 'RenamedEditor.exe'
Copy-Item -LiteralPath $taskExe -Destination $taskCopy
if ((Get-FileHash -LiteralPath $taskExe).Hash -ne (Get-FileHash -LiteralPath $taskCopy).Hash) { throw 'Formal EXE copy hash mismatch.' }
$taskIni = Join-Path $taskRoot 'simple_editor_tool.ini'
[IO.File]::WriteAllText($taskIni,'Invalid settings deliberately retained by the headless fixture.',(New-Object Text.UTF8Encoding($false)))
$taskIniHash = (Get-FileHash -LiteralPath $taskIni).Hash
$taskLeft = Join-Path $taskRoot 'left file.bin'
$taskRight = Join-Path $taskRoot 'right file.bin'
[IO.File]::WriteAllBytes($taskLeft,[byte[]](65,66,67))
[IO.File]::WriteAllBytes($taskRight,[byte[]](65,66,67))
$taskIndex = 0
function Invoke-OwnedCli([string[]]$Arguments,[int]$Expected) {
    $script:taskIndex++
    $taskArgs = @($Arguments | ForEach-Object { if ($_ -match '"') { throw 'Fixture arguments cannot contain quotes.' }; '"' + $_ + '"' })
    $taskOut = Join-Path $taskRoot ("stdout-$taskIndex.txt")
    $taskErr = Join-Path $taskRoot ("stderr-$taskIndex.txt")
    $taskStart = New-Object Diagnostics.ProcessStartInfo
    $taskStart.FileName = $taskCopy
    $taskStart.Arguments = $taskArgs -join ' '
    $taskStart.UseShellExecute = $false
    $taskStart.CreateNoWindow = $true
    $taskStart.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $taskStart.RedirectStandardOutput = $true
    $taskStart.RedirectStandardError = $true
    $taskStart.StandardOutputEncoding = New-Object Text.UTF8Encoding($false)
    $taskStart.StandardErrorEncoding = New-Object Text.UTF8Encoding($false)
    $taskProcess = New-Object Diagnostics.Process
    $taskProcess.StartInfo = $taskStart
    [void]$taskProcess.Start()
    $taskReadOut = $taskProcess.StandardOutput.ReadToEndAsync()
    $taskReadErr = $taskProcess.StandardError.ReadToEndAsync()
    if (-not $taskProcess.WaitForExit(30000)) {
        $taskCurrent = Get-CimInstance Win32_Process -Filter "ProcessId=$($taskProcess.Id)"
        if ($taskCurrent.ExecutablePath -eq $taskCopy) { Stop-Process -Id $taskProcess.Id }
        throw 'Owned CLI fixture timed out; its isolated process was stopped.'
    }
    [IO.File]::WriteAllText($taskOut,$taskReadOut.Result,(New-Object Text.UTF8Encoding($false)))
    [IO.File]::WriteAllText($taskErr,$taskReadErr.Result,(New-Object Text.UTF8Encoding($false)))
    if ($taskProcess.ExitCode -ne $Expected) { throw ("CLI case $taskIndex expected $Expected, got " + $taskProcess.ExitCode + '. ' + [IO.File]::ReadAllText($taskErr)) }
    Write-Output "PASS: formal CLI case $taskIndex exit $Expected"
}
Invoke-OwnedCli @('--help') 0
Invoke-OwnedCli @('--compare','--left',$taskLeft,'--right',$taskRight) 0
[IO.File]::WriteAllBytes($taskRight,[byte[]](65,33,67))
Invoke-OwnedCli @('--compare','--left',$taskLeft,'--right',$taskRight) 1
$taskReport = Join-Path $taskRoot 'differences.csv'
Invoke-OwnedCli @('--compare','--left',$taskLeft,'--right',$taskRight,'--report',$taskReport) 1
Invoke-OwnedCli @('--compare','--left',$taskLeft,'--right',$taskRight,'--report',$taskReport) 2
Invoke-OwnedCli @('--compare','--left',$taskLeft,'--right',$taskRight,'--report',$taskReport,'--overwrite') 1
Invoke-OwnedCli @('--compare','--left',$taskLeft) 2
if ((Get-FileHash -LiteralPath $taskIni).Hash -ne $taskIniHash) { throw 'Headless invocation modified isolated GUI settings.' }
if ([IO.File]::ReadAllText($taskReport).IndexOf('changed') -lt 0) { throw 'Formal CLI report is incomplete.' }
Write-Output "PASS: byte-matched formal EXE, spaced arguments, reports and unchanged GUI INI. Evidence: $taskRoot"
