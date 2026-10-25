$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
$artifactRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'artifacts'))
$objectRoot = [IO.Path]::GetFullPath((Join-Path $artifactRoot 'obj'))
if ([IO.Path]::GetDirectoryName($artifactRoot) -ne $projectRoot -or
    [IO.Path]::GetDirectoryName($objectRoot) -ne $artifactRoot) { throw 'Invalid intermediate root.' }

$ancestor = $objectRoot
while ($ancestor) {
    if (Test-Path -LiteralPath $ancestor) {
        if (((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to traverse a reparse point: $ancestor"
        }
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
if (-not (Test-Path -LiteralPath $objectRoot)) { Write-Host 'No build intermediates.'; exit 0 }
$pending = [Collections.Generic.Stack[string]]::new()
$pending.Push($objectRoot)
$files = @()
$directories = @()
while ($pending.Count) {
    $item = Get-Item -LiteralPath $pending.Pop() -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to traverse a reparse point: $($item.FullName)"
    }
    if ($item.PSIsContainer) {
        $directories += $item
        foreach ($child in Get-ChildItem -LiteralPath $item.FullName -Force) {
            $pending.Push($child.FullName)
        }
    } else { $files += $item }
}

$objectPrefix = $objectRoot + '\'
$nativeExtensions = @('.obj', '.pch', '.idb', '.pdb', '.ilk', '.res', '.iobj',
    '.ipdb', '.tlog', '.lastbuildstate', '.lib', '.exp')
$removed = 0
foreach ($file in $files) {
    $inObjects = $file.FullName.StartsWith($objectPrefix, [StringComparison]::OrdinalIgnoreCase)
    $extension = $file.Extension.ToLowerInvariant()
    $known = $inObjects -and $extension -in $nativeExtensions
    if ($known) {
        Remove-Item -LiteralPath $file.FullName -Force
        $removed++
    }
}
foreach ($directory in $directories | Sort-Object { $_.FullName.Length } -Descending) {
    if (($directory.FullName -eq $objectRoot -or
        $directory.FullName.StartsWith($objectPrefix, [StringComparison]::OrdinalIgnoreCase)) -and
        @(Get-ChildItem -LiteralPath $directory.FullName -Force).Count -eq 0) {
        Remove-Item -LiteralPath $directory.FullName -Force
    }
}
Write-Host "Clean complete: $removed allowlisted compiler files removed."
Write-Host 'Preserved EXE/INI/DLL/data/log/evidence/unknown files, including nested delivery folders.'

