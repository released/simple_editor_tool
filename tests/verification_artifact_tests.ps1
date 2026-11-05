$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\scripts\verification_artifacts.ps1')
$passed = 0
function Check([bool]$Ok,[string]$Name) {
    if (-not $Ok) { throw "FAIL: $Name" }
    $script:passed++; Write-Output "PASS: $Name"
}
function Reject([scriptblock]$Action,[string]$Name) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true }
    Check $rejected $Name
}
function Write-SyntheticOutputs($Run) {
    $obj = Join-Path $Run.Root 'obj\NativeCoreTests'
    [void](New-Item -ItemType Directory -Path $obj -Force)
    [IO.File]::WriteAllText((Join-Path $obj 'fixture.obj'),'synthetic compiler object')
    [IO.File]::WriteAllBytes((Join-Path $Run.Root 'bin\NativeCoreTests.exe'),[byte[]](1,2,3,4))
    [IO.File]::WriteAllText((Join-Path $Run.Root 'tmp\fixture.ini'),'synthetic native input')
}
$oldTemp = $env:TEMP; $oldTmp = $env:TMP
foreach ($outcome in @('PASS','FAIL','CANCELLED')) {
    $result = Invoke-VerificationLifecycle Core Release {
        param($run,$report)
        Write-SyntheticOutputs $run
        if ($outcome -eq 'FAIL') { throw 'Expected synthetic failure.' }
        if ($outcome -eq 'CANCELLED') { throw [OperationCanceledException]::new('Expected cancellation.') }
    }
    Check ($result.Outcome -eq $outcome -and $result.Cleanup -eq 'PASS' -and -not (Test-Path -LiteralPath $result.Root)) "$outcome finalizer retires EXE/object/fixture and run root"
}
Check ($env:TEMP -eq $oldTemp -and $env:TMP -eq $oldTmp) 'TEMP/TMP restored after every outcome'

$incomplete = Invoke-VerificationLifecycle Core Release {
    param($run,$report)
    Write-SyntheticOutputs $run
    [IO.File]::WriteAllText((Join-Path $run.Root 'unknown-user.ini'),'synthetic retained data')
}
try {
    Check ($incomplete.Outcome -eq 'INCOMPLETE' -and $incomplete.RemainingFiles -eq 5 -and $incomplete.RemainingBytes -gt 0) 'cleanup failure reports actual retained file count and bytes'
    Check (@($incomplete.RetainedEntries | Where-Object Path -EQ 'unknown-user.ini').Count -eq 1 -and $null -eq $incomplete.FilesRemoved) 'incomplete summary lists exact retained paths without invented removal counts'
} finally {
    $sentinel = Join-Path $incomplete.Root 'unknown-user.ini'
    $null = Assert-ArtifactPath $sentinel $incomplete.Root
    Remove-Item -LiteralPath $sentinel -Force
    Clear-VerificationRun $incomplete.Root -Apply | Out-Null
}

$run = New-VerificationRun Core Release
try {
    Write-SyntheticOutputs $run
    Reject { Clear-VerificationRun $run.Root -Apply } 'active unsealed runs cannot be retired'
    Seal-VerificationRun $run
    $plan = Clear-VerificationRun $run.Root
    Check ($plan.Files.Count -eq 3 -and (Test-Path -LiteralPath (Join-Path $run.Root 'bin\NativeCoreTests.exe'))) 'preview is read-only and includes the test EXE'
    $object = Join-Path $run.Root 'obj\NativeCoreTests\fixture.obj'
    [IO.File]::WriteAllText($object,'changed after registration')
    Reject { Clear-VerificationRun $run.Root -Apply } 'changed hash rejects before any test EXE deletion'
    Check (Test-Path -LiteralPath (Join-Path $run.Root 'bin\NativeCoreTests.exe')) 'other registered files survive failed whole-run preflight'
    Seal-VerificationRun $run
    $original = $run.Files
    $run.Files = @([pscustomobject]@{Path='..\..\build\SimpleEditorTool.exe';Bytes=4;Sha256=('A'*64)})
    Write-VerificationManifest $run
    Reject { Clear-VerificationRun $run.Root -Apply } 'manifest traversal cannot target delivery or outside paths'
    $run.Files = @($original[0],$original[0]); Write-VerificationManifest $run
    Reject { Clear-VerificationRun $run.Root -Apply } 'duplicate manifest paths are rejected'
    $run.Files = $original; Write-VerificationManifest $run
    $sentinel = Join-Path $run.Root 'unknown-user.ini'
    [IO.File]::WriteAllText($sentinel,'synthetic preservation sentinel')
    Reject { Clear-VerificationRun $run.Root -Apply } 'unknown additions make cleanup incomplete'
    Check ([IO.File]::ReadAllText($sentinel) -eq 'synthetic preservation sentinel') 'unknown INI retained byte-for-byte'
    Check (Test-Path -LiteralPath (Join-Path $run.Root 'bin\NativeCoreTests.exe')) 'unknown files block the entire run before any deletion'
    # This exact sentinel was created above by this test, never by a user.
    $null = Assert-ArtifactPath $sentinel $run.Root
    Remove-Item -LiteralPath $sentinel -Force
} finally {
    if (Test-Path -LiteralPath $run.Root) { Clear-VerificationRun $run.Root -Apply | Out-Null }
}
foreach ($bad in @($script:artifactProjectRoot,[IO.Path]::GetPathRoot($script:artifactProjectRoot),
    (Split-Path $script:artifactProjectRoot -Parent),$env:SystemRoot,[IO.Path]::GetTempPath(),
    (Join-Path $script:verificationBase '..\..\..\outside'))) {
    Reject { Resolve-VerificationRun $bad } 'project/system/TEMP/parent/traversal root rejected without touching it'
}
$left = New-VerificationRun Core Debug; $right = New-VerificationRun Core Debug
$link = Join-Path $left.Root 'obj\junction'
try {
    Write-SyntheticOutputs $left; Write-SyntheticOutputs $right
    Seal-VerificationRun $left
    [void](New-Item -ItemType Junction -Path $link -Target $right.Root)
    Reject { Clear-VerificationRun $left.Root -Apply } 'junction rejected before registered output deletion'
    Check (Test-Path -LiteralPath (Join-Path $right.Root 'bin\NativeCoreTests.exe')) 'junction target is preserved'
} finally {
    # Nonrecursive unlink of the exact synthetic junction; both roots are owned.
    if (Test-Path -LiteralPath $link) { [IO.Directory]::Delete($link) }
    foreach ($owned in @($left,$right)) { Seal-VerificationRun $owned; Clear-VerificationRun $owned.Root -Apply | Out-Null }
}
Write-Output "PASS: $passed verification-retention assertions; no current test payload remains."
