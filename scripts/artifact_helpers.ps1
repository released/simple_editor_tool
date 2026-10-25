$script:artifactProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
if (-not (Test-Path -LiteralPath (Join-Path $script:artifactProjectRoot 'SimpleEditorTool.vcxproj') -PathType Leaf)) {
    throw 'Artifact helpers must run from the intended Simple Editor Tool project.'
}

function Assert-ArtifactPath([string]$Path, [string]$Root) {
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    if (-not $base.StartsWith($script:artifactProjectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup root must be strictly inside the script's project: $base"
    }
    if ($full -ne $base -and -not $full.StartsWith($base + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path leaves the intended artifact directory: $full"
    }
    $ancestor = $full
    while ($ancestor) {
        if (Test-Path -LiteralPath $ancestor) {
            $item = Get-Item -LiteralPath $ancestor -Force -ErrorAction Stop
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing a reparse point: $ancestor"
            }
        }
        $ancestor = [IO.Path]::GetDirectoryName($ancestor)
    }
    return $full
}

function Get-ArtifactInventory([string]$Root) {
    $base = Assert-ArtifactPath $Root $Root
    $files = [Collections.Generic.List[object]]::new()
    $directories = [Collections.Generic.List[object]]::new()
    $pending = [Collections.Generic.Stack[string]]::new()
    if (Test-Path -LiteralPath $base) { $pending.Push($base) }
    while ($pending.Count) {
        $item = Get-Item -LiteralPath $pending.Pop() -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Refusing a reparse point: $($item.FullName)" }
        if ($item.PSIsContainer) {
            $directories.Add($item)
            foreach ($child in Get-ChildItem -LiteralPath $item.FullName -Force -ErrorAction Stop) { $pending.Push($child.FullName) }
        } else { $files.Add($item) }
    }
    return [pscustomobject]@{ Files = $files; Directories = $directories }
}

function Test-CliFixture([string]$Path) {
    if ([IO.Path]::GetFileName($Path) -notmatch '^cli-owned-[a-f0-9]{32}$') { return $false }
    $marker = Join-Path $Path '.simple-editor-cli-fixture'
    if (Test-Path -LiteralPath $marker -PathType Leaf) {
        return (Get-Item -LiteralPath $marker).Length -lt 1024 -and [IO.File]::ReadAllText($marker) -eq 'SimpleEditorTool CLI fixture v1'
    }
    $ini = Join-Path $Path 'simple_editor_tool.ini'
    return (Test-Path -LiteralPath $ini -PathType Leaf) -and (Get-Item -LiteralPath $ini).Length -lt 1024 -and
        [IO.File]::ReadAllText($ini) -eq 'Invalid settings deliberately retained by the headless fixture.'
}

function Test-CliFixtureFile([string]$Name) {
    return $Name -in @('.simple-editor-cli-fixture', 'RenamedEditor.exe', 'simple_editor_tool.ini', 'left file.bin', 'right file.bin', 'differences.csv') -or
        $Name -match '^(stdout|stderr)-[1-7]\.txt$'
}

function Assert-NoArtifactProcess([string]$Root) {
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    foreach ($process in Get-Process) {
        try { $path = $process.Path } catch { continue }
        if ($path -and $path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Artifact executable is running; it was left untouched (PID $($process.Id))."
        }
    }
}

function Get-ArtifactShellTarget {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Software\Classes\*\shell\SimpleEditorTool.Open')
    if (-not $key) { return $null }
    try {
        $target = $key.GetValue('SimpleEditorTool.Executable')
        if ($target -is [string] -and $target.Length) { return [IO.Path]::GetFullPath($target) }
    } finally { $key.Dispose() }
}

function Remove-EmptyArtifactDirectories($Directories, [string]$Root) {
    foreach ($directory in $Directories | Sort-Object { $_.FullName.Length } -Descending) {
        $path = Assert-ArtifactPath $directory.FullName $Root
        if ((Test-Path -LiteralPath $path -PathType Container) -and @(Get-ChildItem -LiteralPath $path -Force).Count -eq 0) {
            Remove-Item -LiteralPath $path -Force -ErrorAction Stop
        }
    }
}

function Remove-OwnedCliFixture([string]$Path, [string]$TestsRoot) {
    $full = Assert-ArtifactPath $Path $TestsRoot
    if ([IO.Path]::GetDirectoryName($full) -ne [IO.Path]::GetFullPath($TestsRoot).TrimEnd('\')) { throw 'CLI fixture must be directly inside artifacts/tests.' }
    if (-not (Test-Path -LiteralPath $full)) { return }
    $inventory = Get-ArtifactInventory $full
    if (-not (Test-CliFixture $full)) {
        if ($inventory.Files.Count -eq 0) { Remove-EmptyArtifactDirectories $inventory.Directories $full; return }
        throw 'CLI fixture ownership marker does not match.'
    }
    Assert-NoArtifactProcess $full
    foreach ($file in $inventory.Files) {
        if ($file.DirectoryName -eq $full -and (Test-CliFixtureFile $file.Name)) {
            $null = Assert-ArtifactPath $file.FullName $full
            Remove-Item -LiteralPath $file.FullName -Force -ErrorAction Stop
        }
    }
    Remove-EmptyArtifactDirectories $inventory.Directories $full
    if (Test-Path -LiteralPath $full) { Write-Warning "Unknown CLI fixture files were retained: $full" }
}
