[CmdletBinding()]
param(
    [ValidateSet('Core','Ui')][string]$Suite = 'Core',
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [string[]]$Mode = @('--localization'),
    [string]$CaptureDirectory,
    [ValidateRange(1,1800)][int]$TimeoutSeconds = 900
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'verification_artifacts.ps1')
$project = $script:artifactProjectRoot
$projectName = if ($Suite -eq 'Core') {'NativeCoreTests'} else {'NativeUiTests'}
$source = [IO.File]::ReadAllText((Join-Path $project ('tests\native_' + $Suite.ToLowerInvariant() + '_tests.cpp')))
foreach ($item in $Mode) {
    if ($item -notmatch '^--[a-z][a-z0-9-]{0,63}$' -or -not $source.Contains('L"' + $item + '"') -or $item -eq '--workspace-process') {
        throw "Unsupported focused mode: $item. Process smoke with an extra formal EXE argument uses its dedicated runner."
    }
}
if (-not $Mode.Count) { throw 'Select at least one focused mode.' }
$capture = $null
if ($CaptureDirectory) {
    if ($Suite -ne 'Ui' -or @($Mode | Where-Object {$_ -eq '--markdown-preview'}).Count -ne 1) { throw 'A native capture requires exactly one Ui --markdown-preview group.' }
    $capture = Assert-ArtifactPath ([IO.Path]::GetFullPath($CaptureDirectory)) (Join-Path $project 'docs\verification')
    if ($capture -eq (Join-Path $project 'docs\verification') -or (Test-Path -LiteralPath $capture)) { throw 'Capture directory must be fresh inside docs/verification.' }
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'Visual Studio C++/MFC MSBuild is required.' }

function Invoke-OwnedVerificationProcess([string]$Executable,[string[]]$Arguments) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $Executable; $start.WorkingDirectory = $project
    $start.Arguments = (@($Arguments | ForEach-Object {
        if ($_ -match '["\r\n]') { throw 'Unsupported process argument.' }
        '"' + [regex]::Replace($_,'(\\+)$','$1$1') + '"'
    }) -join ' ')
    $start.UseShellExecute = $false; $start.CreateNoWindow = $true
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
    $process = [Diagnostics.Process]::new(); $process.StartInfo = $start
    $started = $false
    try {
        [void]$process.Start(); $started = $true; $startedAt = $process.StartTime.ToUniversalTime().Ticks
        $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync()
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while (-not $process.WaitForExit(250)) { if ((Get-Date) -gt $deadline) { throw 'Owned verification process timed out.' } }
        $lines = @(($stdout.Result + "`n" + $stderr.Result) -split '\r?\n' | Where-Object { $_ })
        return [pscustomobject]@{ExitCode=$process.ExitCode;PassLines=@($lines | Where-Object {$_ -match '^PASS:'}).Count;Failure=@($lines | Where-Object {$_ -match '^FAIL:'} | Select-Object -First 40);Tail=@($lines | Select-Object -Last 12)}
    } finally {
        if ($started -and -not $process.HasExited) {
            $current = Get-Process -Id $process.Id -ErrorAction SilentlyContinue
            if ($current -and $current.StartTime.ToUniversalTime().Ticks -eq $startedAt -and
                [string]::Equals($current.Path,$Executable,[StringComparison]::OrdinalIgnoreCase)) {
                # This PID and its build/test children were started by this invocation.
                & (Join-Path $env:SystemRoot 'System32\taskkill.exe') /PID $process.Id /T /F | Out-Host
                if (-not $process.WaitForExit(10000)) { throw 'Owned worker could not stop; cleanup remains incomplete.' }
            } else { throw 'Worker identity changed; no unrelated process was stopped.' }
        }
        $process.Dispose()
    }
}

$oldCl = $env:CL
try {
    $env:CL = '/MP2'
    $result = Invoke-VerificationLifecycle -Suite $Suite -Configuration $Configuration -Action {
        param($run,$report)
        $outDir = (Join-Path $run.Root 'bin') + '/'
        $buildProjects = if ($Suite -eq 'Ui') {@('NativeEditor',$projectName)} else {@($projectName)}
        foreach ($name in $buildProjects) {
            $intDir = (Join-Path $run.Root ('obj\' + $name)) + '/'
            $arguments = @((Join-Path $project ($name + '.vcxproj')),('/p:Configuration=' + $Configuration),'/p:Platform=x64',
                ('/p:OutDir=' + $outDir),('/p:IntDir=' + $intDir),'/p:BuildProjectReferences=false','/m:2','/nr:false','/v:minimal')
            $built = Invoke-OwnedVerificationProcess $msbuild $arguments
            if ($built.ExitCode -ne 0) { throw ('Build failed: ' + ($built.Tail -join "`n")) }
            Write-Host "Built isolated $name $Configuration."
        }
        $exe = Join-Path $outDir ($projectName + '.exe')
        foreach ($item in $Mode) {
            $testArguments = @($item); if ($capture -and $item -eq '--markdown-preview') { $testArguments += $capture }
            $tested = Invoke-OwnedVerificationProcess $exe $testArguments
            if ($capture -and $item -eq '--markdown-preview' -and (Test-Path -LiteralPath (Join-Path $capture 'markdown-preview.png'))) {
                $report | Add-Member -NotePropertyName Capture -NotePropertyValue @{Path=(Join-Path $capture 'markdown-preview.png');Sha256=(Get-FileHash (Join-Path $capture 'markdown-preview.png')).Hash} -Force
            }
            $report.Groups += [pscustomobject]@{Mode=$item;ExitCode=$tested.ExitCode;PassLines=$tested.PassLines;Failure=$tested.Failure;ExeSha256=(Get-FileHash -LiteralPath $exe).Hash}
            if ($tested.ExitCode -ne 0) { throw ('Native test failed: ' + ((@($tested.Failure) + $tested.Tail) -join "`n")) }
            Write-Host "PASS: $Suite $Configuration $item ($($tested.PassLines) PASS lines including summary)."
        }
    }
} finally { $env:CL = $oldCl }
$summary = [pscustomobject]@{Owner='simple_editor.native-verification.v1';Suite=$Suite;Configuration=$Configuration;
    DateUtc=(Get-Date).ToUniversalTime().ToString('o');Result=$result}
$suffix = if ($result.Outcome -eq 'PASS') {'pass'} else {'fail'}
$summaryPath = Join-Path $project ("docs\verification\native-$Suite-$Configuration-$suffix.json")
if (Test-Path -LiteralPath $summaryPath) {
    if (([IO.File]::ReadAllText($summaryPath) | ConvertFrom-Json).Owner -ne $summary.Owner) { throw 'Unknown prior summary preserved.' }
}
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7),[Text.UTF8Encoding]::new($false))
if ($result.Cleanup -eq 'PASS') {
    Write-Output ("{0}: cleanup=PASS; removed {1} files / {2} bytes. Summary: {3}" -f
        $result.Outcome,$result.FilesRemoved,$result.BytesRemoved,$summaryPath)
} else {
    Write-Output ("INCOMPLETE: {0}; retained {1} files / {2} bytes. Details: {3}" -f
        $result.Cleanup,$result.RemainingFiles,$result.RemainingBytes,$summaryPath)
}
if ($result.Outcome -ne 'PASS' -or $result.Cleanup -ne 'PASS') { throw ($result.Error + ' ' + $result.Cleanup) }
