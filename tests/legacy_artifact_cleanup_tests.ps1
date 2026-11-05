$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $project 'scripts\artifact_helpers.ps1')
$fixtureParent = Join-Path $project 'artifacts\cleanup-rules-checks'
$fixture = Join-Path $fixtureParent ('legacy-' + [Guid]::NewGuid().ToString('N'))
$null = Assert-ArtifactPath $fixture $fixtureParent
if (Test-Path -LiteralPath $fixture) { throw 'Fixture already exists.' }
$null = New-Item -ItemType Directory -Path $fixture
$owned = @{}; $checks = [Collections.Generic.List[string]]::new()
$failure = ''; $cleanup = 'PASS'; $removed = 0; $removedBytes = [long]0
$oldTemp = $env:TEMP; $oldTmp = $env:TMP
function Put([string]$Relative,[string]$Text) {
    $path = Assert-ArtifactPath (Join-Path $fixture $Relative) $fixture
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
    [IO.File]::WriteAllText($path,$Text,[Text.UTF8Encoding]::new($false))
    $owned[$path] = (Get-FileHash -LiteralPath $path).Hash
}
function Check([bool]$Value,[string]$Label) {
    if (-not $Value) { throw "FAIL: $Label" }
    $checks.Add($Label); Write-Host "PASS: $Label"
}
function Save-Record($Value,[string]$Name = 'artifacts-cleanup-synthetic.json') {
    Put ('docs\verification\' + $Name) ($Value | ConvertTo-Json -Depth 7)
}
function Purge([bool]$Apply = $false) {
    if ($Apply) { return @(& (Join-Path $fixture 'scripts\purge_artifacts.ps1') -Apply) }
    return @(& (Join-Path $fixture 'scripts\purge_artifacts.ps1'))
}
function Rejected([scriptblock]$Action,[string]$Label) {
    $rejected = $false; try { $null = & $Action } catch { $rejected = $true }
    Check $rejected $Label
}
try {
    Put '.cleanup-fixture-owner' 'simple_editor.legacy_cleanup.tests.v1'
    [void][IO.Directory]::CreateDirectory((Join-Path $fixture 'tmp'))
    $env:TEMP = Join-Path $fixture 'tmp'; $env:TMP = $env:TEMP
    foreach ($name in @('artifact_helpers.ps1','purge_artifacts.ps1','clean.bat','clean.ps1','clean_intermediates.ps1')) {
        Put ('scripts\' + $name) ([IO.File]::ReadAllText((Join-Path $project ('scripts\' + $name))))
    }
    Put 'SimpleEditorTool.vcxproj' '<Project />'
    Put 'src\sentinel.cpp' 'source sentinel'
    Put 'build\SimpleEditorTool.exe' 'formal executable sentinel'
    Put 'build\settings.ini' 'personal preference sentinel'
    Put 'build\required.dll' 'runtime sentinel'
    Put 'artifacts\legacy\old-test.exe' 'synthetic test binary'
    Put 'artifacts\legacy\fixture.bin' 'synthetic fixture'
    Put 'artifacts\legacy\Customer.exe' 'unlisted executable'
    Put 'artifacts\legacy\private.bin' 'unlisted binary'
    Put 'artifacts\legacy\user.log' 'unlisted log'
    Put 'artifacts\legacy\settings.ini' 'unlisted settings'
    Put 'artifacts\obj\Release\compiler.obj' 'compiler sentinel'
    Put 'docs\verification\acceptance.json' '{"result":"PASS","synthetic":true}'
    $protect = @('src\sentinel.cpp','build\SimpleEditorTool.exe','build\settings.ini','build\required.dll',
        'artifacts\legacy\Customer.exe','artifacts\legacy\private.bin','artifacts\legacy\user.log','artifacts\legacy\settings.ini')
    $record = [pscustomobject]@{Owner='simple_editor.artifacts-cleanup-audit.v1';Root=(Join-Path $fixture 'artifacts');
        Status='REVIEWED_FOR_PURGE';VerificationComplete=$true;DisposableTestData=$true;RecoveryNeeded=$false;Unknown=@();
        Evidence=@{Path='docs\verification\acceptance.json';Sha256=(Get-FileHash (Join-Path $fixture 'docs\verification\acceptance.json')).Hash};Files=@()}
    foreach ($relative in @('artifacts\legacy\old-test.exe','artifacts\legacy\fixture.bin')) {
        $file = Get-Item -LiteralPath (Join-Path $fixture $relative)
        $record.Files += [pscustomobject]@{Path=$relative;Bytes=$file.Length;Sha256=(Get-FileHash $file.FullName).Hash;Provenance='Fresh synthetic fixture owned by this test'}
    }
    Save-Record $record
    $preview = Purge
    Check (($preview -match '^REMOVE legacy\\').Count -eq 2 -and (Test-Path (Join-Path $fixture $record.Files[0].Path))) 'preview recognizes reviewed old EXE/data without deleting'
    $batch = Join-Path $fixture 'scripts\clean.bat'
    $null = & $batch /purge
    Check ($LASTEXITCODE -eq 0 -and (Test-Path (Join-Path $fixture $record.Files[1].Path))) 'clean /purge dispatch is read-only'
    $null = & $batch /purge /invalid
    Check ($LASTEXITCODE -eq 2 -and (Test-Path (Join-Path $fixture $record.Files[0].Path))) 'invalid purge arguments return2 without deletion'
    $null = & $batch /purge /apply
    Check ($LASTEXITCODE -eq 0 -and -not (Test-Path (Join-Path $fixture $record.Files[0].Path)) -and
        -not (Test-Path (Join-Path $fixture $record.Files[1].Path))) 'explicit purge removes old hash-matched test-only EXE/data'
    $null = Purge $true
    Check (@(Purge | Where-Object {$_ -match '^REMOVE '}).Count -eq 0) 'repeated purge is idempotent'
    $same = $true; foreach ($relative in $protect) { $path = Join-Path $fixture $relative; $same = $same -and (Get-FileHash $path).Hash -eq $owned[$path] }
    Check $same 'formal EXE/INI/DLL, source and unlisted legacy EXE/bin/log/INI keep hashes'
    Put 'artifacts\legacy\old-test.exe' 'synthetic test binary'
    Put 'artifacts\legacy\fixture.bin' 'synthetic fixture'
    Put 'artifacts\obj\Release\compiler.obj' 'compiler sentinel'
    Put 'artifacts\legacy\old-test.exe' 'Xynthetic test binary'
    Rejected { Purge $true } 'same-length changed historical data refuses the whole batch'
    Check (Test-Path (Join-Path $fixture 'artifacts\obj\Release\compiler.obj')) 'hash preflight keeps even ordinary compiler candidates on refusal'
    Put 'artifacts\legacy\old-test.exe' 'synthetic test binary'
    Put 'docs\verification\acceptance.json' '{"result":"FAIL","synthetic":true}'
    Rejected { Purge $true } 'changed compact verification evidence refuses deletion'
    Put 'docs\verification\acceptance.json' '{"result":"PASS","synthetic":true}'
    foreach ($field in @('VerificationComplete','DisposableTestData')) {
        $record.$field = $false; Save-Record $record
        Rejected { Purge $true } ($field + ' is required before old output can be retired')
        $record.$field = $true
    }
    $record.RecoveryNeeded = $true; Save-Record $record
    Rejected { Purge $true } 'unresolved recovery archives remain protected'
    $record.RecoveryNeeded = $false; $record.Unknown = @('unresolved data'); Save-Record $record
    Rejected { Purge $true } 'unresolved unknown ownership refuses deletion'
    $record.Unknown = @(); $record.Status = 'AUDIT_ONLY'; Save-Record $record
    $null = Purge $true
    Check (Test-Path (Join-Path $fixture $record.Files[0].Path)) 'audit-only records never authorize deletion'
    $record.Status = 'REVIEWED_FOR_PURGE'
    $originalPath = $record.Files[0].Path
    foreach ($bad in @('build\SimpleEditorTool.exe','artifacts\..\build\SimpleEditorTool.exe',$env:SystemRoot,
        'artifacts\verification\run-00000000000000000000000000000000\bin\NativeUiTests.exe')) {
        $record.Files[0].Path = $bad; Save-Record $record
        Rejected { Purge $true } ('reject retargeted or registry-owned path: ' + $bad)
    }
    $record.Files[0].Path = $originalPath; $originalRoot = $record.Root
    $record.Root = Join-Path $project 'artifacts'; Save-Record $record
    Rejected { Purge $true } 'copied helper independently anchors its own fixture project'
    $record.Root = $originalRoot; Save-Record $record
    $conflict = ($record | ConvertTo-Json -Depth 7 | ConvertFrom-Json); $conflict.Files[0].Sha256 = ('0' * 64)
    Save-Record $conflict 'artifacts-cleanup-conflict.json'
    Rejected { Purge $true } 'conflicting old-output records refuse before deletion'
    Remove-Item -LiteralPath (Join-Path $fixture 'docs\verification\artifacts-cleanup-conflict.json') -Force
    $denial = [pscustomobject]@{Owner=$record.Owner;Root=$record.Root;Status='REVIEWED_FOR_PURGE';
        CurrentRejection=@{Reason='blocked by policy'};Files=@($record.Files[0])}
    Save-Record $denial 'artifacts-cleanup-denied.json'
    $preview = Purge
    Check (@($preview | Where-Object {$_ -like 'BLOCKED previous-review legacy\old-test.exe'}).Count -eq 1) 'previous rejection is visible in preview even with an approved duplicate'
    Rejected { Purge $true } 'a previous review rejection overrides all purge rules'
    Check (Test-Path (Join-Path $fixture $record.Files[1].Path)) 'review rejection also preserves other candidates before dispatch'
    Remove-Item -LiteralPath (Join-Path $fixture 'docs\verification\artifacts-cleanup-denied.json') -Force
    $helperOriginal = [IO.File]::ReadAllText((Join-Path $fixture 'scripts\artifact_helpers.ps1'))
    $runtimePath = (Join-Path $fixture $record.Files[0].Path).Replace("'","''")
    Put 'scripts\artifact_helpers.ps1' ($helperOriginal + "`nfunction Get-ArtifactShellTarget { return '" + $runtimePath + "' }`n")
    $null = Purge $true
    Check (Test-Path (Join-Path $fixture $record.Files[0].Path)) 'runtime integration target stays protected despite a matching old-output record'
    Put 'scripts\artifact_helpers.ps1' ($helperOriginal + "`nfunction Assert-NoArtifactProcess { throw 'Synthetic active worker' }`n")
    Put 'artifacts\legacy\fixture.bin' 'synthetic fixture'
    Rejected { Purge $true } 'active-worker refusal remains in the apply path'
    Check (Test-Path (Join-Path $fixture $record.Files[1].Path)) 'active-worker refusal happens before deletion'
    Put 'scripts\artifact_helpers.ps1' $helperOriginal
    Put 'outside\sentinel.txt' 'project-local junction target'
    $link = Join-Path $fixture 'artifacts\junction'
    [void](New-Item -ItemType Junction -Path $link -Target (Join-Path $fixture 'outside'))
    try {
        Rejected { Purge $true } 'reparse escape refuses all cleanup before deletion'
        Check ([IO.File]::ReadAllText((Join-Path $fixture 'outside\sentinel.txt')) -eq 'project-local junction target') 'junction target bytes are unchanged'
    } finally { [IO.Directory]::Delete($link) }
    Put 'artifacts\obj\Release\ordinary.obj' 'ordinary compiler output'
    $null = & $batch
    Check ($LASTEXITCODE -eq 0 -and -not (Test-Path (Join-Path $fixture 'artifacts\obj\Release\ordinary.obj')) -and
        (Test-Path (Join-Path $fixture $record.Files[0].Path))) 'ordinary clean remains compiler-only and never purges old test EXEs'
    $same = $true; foreach ($relative in $protect) { $path = Join-Path $fixture $relative; $same = $same -and (Get-FileHash $path).Hash -eq $owned[$path] }
    Check $same 'all protected fixture hashes still match after every negative/positive path'
} catch { $failure = $_.Exception.Message }
finally {
    $env:TEMP = $oldTemp; $env:TMP = $oldTmp
    try {
        $inventory = Get-ArtifactInventory $fixture
        foreach ($file in $inventory.Files) {
            if (-not $owned.ContainsKey($file.FullName) -or (Get-FileHash $file.FullName).Hash -ne $owned[$file.FullName]) {
                throw 'Unknown/changed fixture file retained; teardown incomplete.'
            }
        }
        foreach ($file in $inventory.Files) {
            $null = Assert-ArtifactPath $file.FullName $fixture
            Remove-Item -LiteralPath $file.FullName -Force
            $removed++; $removedBytes += $file.Length
        }
        Remove-EmptyArtifactDirectories $inventory.Directories $fixture
        if (Test-Path -LiteralPath $fixture) { throw 'Fixture root remains.' }
        if (@(Get-ChildItem -LiteralPath $fixtureParent -Force).Count -eq 0) { Remove-Item -LiteralPath $fixtureParent -Force }
    } catch { $cleanup = $_.Exception.Message }
}
$report = [pscustomobject]@{Owner='simple_editor.legacy-cleanup-tests.v1';DateLocal=(Get-Date -Format o);
    Passed=$checks.Count;Checks=@($checks);Failure=$failure;Cleanup=$cleanup;FilesRemoved=$removed;BytesRemoved=$removedBytes;
    Fixture=$fixture;FixtureAbsent=(-not (Test-Path -LiteralPath $fixture));WorkerCheck='Guard routing mocked; existing process enumeration unchanged'}
$summary = Join-Path $project 'docs\verification\cleanup-rules-tests.json'
if ((Test-Path $summary) -and ([IO.File]::ReadAllText($summary) | ConvertFrom-Json).Owner -ne $report.Owner) { throw 'Unknown summary preserved.' }
[IO.File]::WriteAllText($summary,($report | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
if ($failure -or $cleanup -ne 'PASS') { throw ($failure + ' ' + $cleanup) }
Write-Output "PASS: $($checks.Count) historical cleanup checks; teardown removed$removed files/$removedBytes bytes."
