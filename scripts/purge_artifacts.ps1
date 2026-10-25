[CmdletBinding()]
param([switch]$Apply)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'artifact_helpers.ps1')
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
$root = [IO.Path]::GetFullPath((Join-Path $project 'artifacts'))
if ([IO.Path]::GetDirectoryName($root) -ne $project -or -not (Test-Path -LiteralPath (Join-Path $project 'SimpleEditorTool.vcxproj'))) { throw 'Invalid project artifact root.' }
$inventory = Get-ArtifactInventory $root
$candidates = [Collections.Generic.List[object]]::new()
$retained = [Collections.Generic.List[string]]::new()
$cliOwners = @{}
$shellTarget = Get-ArtifactShellTarget
$native = @('.obj','.pch','.idb','.pdb','.ilk','.res','.iobj','.ipdb','.tlog','.lastbuildstate','.lib','.exp')
$legacyPython = @('Analysis-00.toc','base_library.zip','EXE-00.toc','PKG-00.toc','PYZ-00.pyz','PYZ-00.toc','SimpleEditorTool.pkg','warn-SimpleEditorTool.txt','xref-SimpleEditorTool.html')
$evidence = @('advanced-verification.json','build-layout-after.json','build-layout-before.json','build-layout-moves.json','ctrl-wheel-final-inputs.json','discovery-final-inputs.json','explorer-before.json','explorer-final-inputs.json','explorer-runtime-ini-checkpoint.json','native-workflows-final-inputs.json','workspace-verification.json','zoom-verification.json','ui_smoke.png','windows_ui_125pct_smoke.png','windows_ui_smoke.png','advanced-release-dependencies.txt')
$logPrefix = 'advanced|alignment|async-save|cli|cross|ctrl-wheel|discovery|documents|drop|explorer|final|folder|formal-process|inspection|merge-sync|pager-fit|paging|preferences|progress|recovery|search-tools|structured|template|text-report|text-reports|workflow|workspace|zoom|artifacts-cleanup'
foreach ($file in $inventory.Files) {
    $relative = $file.FullName.Substring($root.Length + 1)
    $known = $false
    if ($relative.StartsWith('obj\',[StringComparison]::OrdinalIgnoreCase)) {
        $known = $file.Extension.ToLowerInvariant() -in $native -or $file.Name -match '^(SimpleEditorTool|NativeEditor|NativeCoreTests|NativeUiTests)(\.(exe|lib)\.recipe|\.Build\.CppClean\.log)$'
        $known = $known -or $relative -eq 'obj\pyinstaller\spec\SimpleEditorTool.spec' -or
            ($file.DirectoryName -eq (Join-Path $root 'obj\pyinstaller\work\SimpleEditorTool') -and $file.Name -in $legacyPython)
    } elseif ($relative -match '^tests\\(Debug|Release)\\NativeUiTests\.(exe|ilk|pdb|lib|exp)$' -or $relative -eq 'tests\NativeCoreTests.exe' -or $relative -eq 'debug\SimpleEditorTool.exe') { $known = $true }
    elseif ($relative -match '^tests\\(cli-owned-[a-f0-9]{32})\\([^\\]+)$') {
        $directory = $matches[1]
        if (-not $cliOwners.ContainsKey($directory)) { $cliOwners[$directory] = Test-CliFixture (Join-Path $root ('tests\' + $directory)) }
        $known = $cliOwners[$directory] -and (Test-CliFixtureFile $file.Name)
    } elseif ($relative -match ('^logs\\(' + $logPrefix + ')-[A-Za-z0-9_.-]+\.(log|txt)$')) { $known = $true }
    elseif ($file.DirectoryName -eq (Join-Path $root 'evidence')) { $known = $file.Name -in $evidence -or $file.Name -match '^advanced-[A-Za-z0-9_.-]+\.log$' }
    elseif ($relative -match '^evidence\\advanced-native-20261025\\(folder-compare|text-compare|three-way-conflict)\.png$') { $known = $true }
    elseif ($relative -match '^screenshots\\alignment\\(binary-template-tree|large-text-page|preferences-dark-untitled(-before-title)?|unicode-intraline-overview(-before-layout)?)\.png$') { $known = $true }
    elseif ($relative -match '^screenshots\\discovery-(debug|release)\\(folder-replacement|hex-range-bookmarks|live-numeric-inspector|log-tail|shared-text-views|table-key-settings)\.png$') { $known = $true }
    elseif ($relative -match '^screenshots\\workflows-(debug|release)\\(command-palette|folder-result-tree|folder-text-rules|hex-decoding|text-change-markers|text-context-collapse)\.png$') { $known = $true }
    elseif ($relative -eq 'dependencies\scintilla567.zip') { $known = (Get-FileHash -LiteralPath $file.FullName).Hash -eq '2309181E59E7DA4659F890DBC128DB7451CCD094748770E7BE674B830494587C' }
    elseif ($relative -eq 'dependencies\lexilla554.zip') { $known = (Get-FileHash -LiteralPath $file.FullName).Hash -eq '5E0A21356E4E2F19D6A38ECD1C8172D5A2B7B4542940568B023386080DB90FE0' }
    elseif ($relative -eq 'baseline\layout-before\SimpleEditorTool.exe') { $known = (Get-FileHash -LiteralPath $file.FullName).Hash -eq '4424768E6F3CCE534D0684C6168C61744A26F38851265EEEEBF2ABA5C9864D95' }
    if ($known -and $shellTarget -eq $file.FullName) { $known = $false }
    if ($known) { $candidates.Add($file) } else { $retained.Add($relative) }
}
$bytes = ($candidates | Measure-Object Length -Sum).Sum
Write-Output ("{0}: {1} recognized generated files; {2:N2} MiB; {3} protected/unknown files retained." -f $(if ($Apply) {'Apply'} else {'Preview'}),$candidates.Count,($bytes / 1MB),$retained.Count)
if (-not $Apply) {
    $candidates | ForEach-Object { Write-Output ('REMOVE ' + $_.FullName.Substring($root.Length + 1)) }
} else {
    Assert-NoArtifactProcess $root
    foreach ($file in $candidates) {
        $path = Assert-ArtifactPath $file.FullName $root
        $current = Get-Item -LiteralPath $path -Force -ErrorAction Stop
        if ($current.Length -ne $file.Length -or $current.LastWriteTimeUtc -ne $file.LastWriteTimeUtc) { throw "Artifact changed during preflight; retained: $path" }
        Remove-Item -LiteralPath $path -Force -ErrorAction Stop
    }
    Remove-EmptyArtifactDirectories $inventory.Directories $root
}
foreach ($path in $retained) { Write-Output ('KEEP ' + $path) }
if (-not $Apply) { Write-Output 'Preview only. Use -Apply to explicitly confirm removal of the recognized generated files.' }
