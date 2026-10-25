param([string]$Executable = (Join-Path $PSScriptRoot '..\build\SimpleEditorTool.exe'))
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $project 'scripts\artifact_helpers.ps1')
$artifactRoot = Join-Path $project 'artifacts'
$artifactCreated = -not (Test-Path -LiteralPath $artifactRoot)
$fixtureParent = Join-Path $artifactRoot 'cleanup-checks'
$parentCreated = -not (Test-Path -LiteralPath $fixtureParent)
$fixture = Join-Path $fixtureParent ('SimpleEditorTool-Prune-' + [Guid]::NewGuid().ToString('N'))
$null = Assert-ArtifactPath $fixture $fixtureParent
$null = New-Item -ItemType Directory -Path (Join-Path $fixture 'scripts')
$passed = 0
function Check([bool]$Value, [string]$Label) {
    if (-not $Value) { throw "FAIL: $Label" }
    $script:passed++; Write-Output "PASS: $Label"
}
function Put([string]$Relative, [string]$Text) {
    $path = Join-Path $fixture $Relative
    $null = [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
    [IO.File]::WriteAllText($path,$Text,[Text.UTF8Encoding]::new($false))
}
try {
    $forbiddenRoots = @(
        @{ Name = 'Windows'; Path = $env:SystemRoot },
        @{ Name = 'System32'; Path = (Join-Path $env:SystemRoot 'System32') },
        @{ Name = 'user profile'; Path = [Environment]::GetFolderPath('UserProfile') },
        @{ Name = 'external TEMP'; Path = [IO.Path]::GetTempPath() },
        @{ Name = 'volume root'; Path = [IO.Path]::GetPathRoot($project) },
        @{ Name = 'workspace parent'; Path = [IO.Path]::GetDirectoryName($project) },
        @{ Name = 'sibling project'; Path = $project + '-sibling' },
        @{ Name = 'whole project'; Path = $project }
    )
    foreach ($case in $forbiddenRoots) {
        $rejected = $false
        try { $null = Assert-ArtifactPath (Join-Path $case.Path '__scope_probe__') $case.Path } catch { $rejected = $true }
        Check $rejected ('caller cannot retarget cleanup to ' + $case.Name)
    }
    foreach ($name in @('artifact_helpers.ps1','purge_artifacts.ps1','verify_cli.ps1')) { Copy-Item -LiteralPath (Join-Path $project ('scripts\'+$name)) -Destination (Join-Path $fixture ('scripts\'+$name)) }
    Put 'SimpleEditorTool.vcxproj' '<Project />'
    Put 'src\sentinel.cpp' 'source sentinel'
    Put 'build\SimpleEditorTool.exe' 'delivery sentinel'
    Put 'build\simple_editor_tool.ini' 'user settings sentinel'
    Put 'artifacts\obj\Release\discard.obj' 'compiler output'
    Put 'artifacts\tests\Release\NativeUiTests.ilk' 'linker output'
    Put 'artifacts\tests\Release\NativeUiTests.exe' 'generated test executable'
    Put 'artifacts\obj\Release\user.ini' 'keep user settings'
    Put 'artifacts\obj\Release\build\Customer.exe' 'keep nested user delivery'
    Put 'artifacts\tests\notes.txt' 'keep unknown notes'
    Put 'artifacts\tests\Release\Customer.exe' 'keep unknown executable'
    Put 'artifacts\logs\private.log' 'keep unknown log'
    $unknown = @('build\SimpleEditorTool.exe','build\simple_editor_tool.ini','src\sentinel.cpp','artifacts\obj\Release\user.ini','artifacts\obj\Release\build\Customer.exe','artifacts\tests\notes.txt','artifacts\tests\Release\Customer.exe','artifacts\logs\private.log')
    $hashes = @{}; foreach ($name in $unknown) { $hashes[$name] = (Get-FileHash -LiteralPath (Join-Path $fixture $name)).Hash }
    $null = & (Join-Path $fixture 'scripts\purge_artifacts.ps1')
    Check (Test-Path -LiteralPath (Join-Path $fixture 'artifacts\obj\Release\discard.obj')) 'default preview deletes nothing'
    $null = & (Join-Path $fixture 'scripts\purge_artifacts.ps1') -Apply
    Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'artifacts\obj\Release\discard.obj')) -and -not (Test-Path -LiteralPath (Join-Path $fixture 'artifacts\tests\Release\NativeUiTests.exe'))) 'explicit purge removes recognized compiler and test outputs'
    $same = $true; foreach ($name in $unknown) { $same = $same -and (Get-FileHash -LiteralPath (Join-Path $fixture $name)).Hash -eq $hashes[$name] }
    Check $same 'delivery, source, user INI and unknown files keep their hashes'
    $null = & (Join-Path $fixture 'scripts\purge_artifacts.ps1') -Apply
    Check ((Get-FileHash -LiteralPath (Join-Path $fixture 'artifacts\tests\notes.txt')).Hash -eq $hashes['artifacts\tests\notes.txt']) 'repeated purge is idempotent and retains unknown files'
    $escaped = $false; try { $null = Assert-ArtifactPath (Join-Path $fixture 'artifacts\..\build\SimpleEditorTool.exe') (Join-Path $fixture 'artifacts') } catch { $escaped = $true }
    Check $escaped 'normalized paths cannot escape the artifact root'
    $unowned = 'artifacts\tests\cli-owned-00000000000000000000000000000000'
    Put ($unowned+'\RenamedEditor.exe') 'unowned executable'
    $null = & (Join-Path $fixture 'scripts\purge_artifacts.ps1') -Apply
    Check ([IO.File]::ReadAllText((Join-Path $fixture ($unowned+'\RenamedEditor.exe'))) -eq 'unowned executable') 'a familiar fixture directory name alone cannot authorize deleting its EXE'
    $empty = Join-Path $fixture 'artifacts\tests\cli-owned-11111111111111111111111111111111'
    $null = New-Item -ItemType Directory -Path $empty
    Remove-OwnedCliFixture $empty (Join-Path $fixture 'artifacts\tests')
    Check (-not (Test-Path -LiteralPath $empty)) 'empty setup failures can be removed without a completed marker'
    Put 'outside\keep.txt' 'junction target sentinel'
    Put 'artifacts\obj\Release\preflight.obj' 'must survive a rejected purge'
    $link = Join-Path $fixture 'artifacts\junction'
    $null = New-Item -ItemType Junction -Path $link -Target (Join-Path $fixture 'outside')
    try {
        $rejected = $false; try { $null = & (Join-Path $fixture 'scripts\purge_artifacts.ps1') -Apply } catch { $rejected = $true }
        Check ($rejected -and (Test-Path -LiteralPath (Join-Path $fixture 'artifacts\obj\Release\preflight.obj')) -and [IO.File]::ReadAllText((Join-Path $fixture 'outside\keep.txt')) -eq 'junction target sentinel') 'junction preflight rejects the entire purge before any deletion'
    } finally { [IO.Directory]::Delete($link) }
    $cliScript = Join-Path $fixture 'scripts\verify_cli.ps1'
    $null = & $cliScript -Executable ([IO.Path]::GetFullPath($Executable))
    Check (@(Get-ChildItem -LiteralPath (Join-Path $fixture 'artifacts\tests') -Directory -Filter 'cli-owned-*').Count -eq 1) 'successful real CLI checks remove their copied EXE and temporary data'
    $failed = $false; try { $null = & $cliScript -Executable (Join-Path $fixture 'does-not-exist.exe') } catch { $failed = $true }
    Check ($failed -and @(Get-ChildItem -LiteralPath (Join-Path $fixture 'artifacts\tests') -Directory -Filter 'cli-owned-*').Count -eq 1) 'failed fixture setup also removes its owned temporary directory'
    $null = & $cliScript -Executable ([IO.Path]::GetFullPath($Executable)) -KeepArtifacts
    $kept = @(Get-ChildItem -LiteralPath (Join-Path $fixture 'artifacts\tests') -Directory -Filter 'cli-owned-*' | Where-Object {Test-CliFixture $_.FullName})
    Check ($kept.Count -eq 1 -and (Test-Path -LiteralPath (Join-Path $kept[0].FullName 'RenamedEditor.exe'))) 'KeepArtifacts explicitly retains diagnostic data'
    [IO.File]::WriteAllText((Join-Path $kept[0].FullName 'user-notes.txt'),'keep notes')
    Remove-OwnedCliFixture $kept[0].FullName (Join-Path $fixture 'artifacts\tests')
    Check ([IO.File]::ReadAllText((Join-Path $kept[0].FullName 'user-notes.txt')) -eq 'keep notes' -and -not (Test-Path -LiteralPath (Join-Path $kept[0].FullName 'RenamedEditor.exe'))) 'per-run cleanup retains unknown notes while removing the owned EXE'
    Write-Output "PASS: $passed artifact cleanup checks"
} finally {
    $expected = [IO.Path]::GetFullPath((Join-Path $fixtureParent ([IO.Path]::GetFileName($fixture))))
    if ($fixture -ne $expected -or [IO.Path]::GetFileName($fixture) -notmatch '^SimpleEditorTool-Prune-[a-f0-9]{32}$') { throw 'Invalid fixture cleanup root.' }
    $inventory = Get-ArtifactInventory $fixture
    foreach ($file in $inventory.Files) { $null = Assert-ArtifactPath $file.FullName $fixture; Remove-Item -LiteralPath $file.FullName -Force }
    Remove-EmptyArtifactDirectories $inventory.Directories $fixture
    foreach ($parent in @(@{ Path = $fixtureParent; Created = $parentCreated }, @{ Path = $artifactRoot; Created = $artifactCreated })) {
        if ($parent.Created -and (Test-Path -LiteralPath $parent.Path -PathType Container) -and @(Get-ChildItem -LiteralPath $parent.Path -Force).Count -eq 0) {
            $null = Assert-ArtifactPath $parent.Path $parent.Path
            Remove-Item -LiteralPath $parent.Path -Force
        }
    }
}
