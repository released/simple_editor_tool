$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'artifact_helpers.ps1')
$script:verificationBase = Join-Path $script:artifactProjectRoot 'artifacts\verification'
$script:verificationOwner = 'simple_editor.verification.v1'

function Resolve-VerificationRun([string]$Path) {
    $full = Assert-ArtifactPath $Path $script:verificationBase
    if ([IO.Path]::GetDirectoryName($full) -ne $script:verificationBase -or
        [IO.Path]::GetFileName($full) -notmatch '^run-[a-f0-9]{32}$') { throw 'Invalid registered verification run.' }
    return $full
}

function Write-VerificationManifest($Run) {
    $root = Resolve-VerificationRun $Run.Root
    [IO.File]::WriteAllText((Join-Path $root '.verification.json'),
        ($Run | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
}

function New-VerificationRun {
    param([ValidateSet('Core','Ui')][string]$Suite, [ValidateSet('Debug','Release')][string]$Configuration)
    $root = Resolve-VerificationRun (Join-Path $script:verificationBase ('run-' + [Guid]::NewGuid().ToString('N')))
    if (Test-Path -LiteralPath $root) { throw 'Verification run already exists.' }
    [void](New-Item -ItemType Directory -Path $root)
    $run = [pscustomobject]@{Owner=$script:verificationOwner;Root=$root;Suite=$Suite;Configuration=$Configuration;
        State='active';Pid=$PID;StartedUtc=(Get-Date).ToUniversalTime().ToString('o');Files=@()}
    Write-VerificationManifest $run
    foreach ($name in @('bin','obj','tmp')) { [void](New-Item -ItemType Directory -Path (Join-Path $root $name)) }
    return $run
}

function Test-VerificationOutput([string]$Relative) {
    if ($Relative -match '^bin\\(Native(Core|Ui)Tests\.(exe|ilk|pdb|lib|exp)|NativeEditor\.(lib|pdb|exp))$') { return $true }
    if ($Relative -match '^obj\\(NativeCoreTests|NativeUiTests|NativeEditor)\\.+') {
        $producer = $Matches[1]
        return [IO.Path]::GetExtension($Relative).ToLowerInvariant() -in @('.obj','.pch','.idb','.pdb','.ilk','.res','.iobj','.ipdb','.tlog','.lastbuildstate','.lib','.exp') -or
            $Relative -match '\\(NativeCoreTests|NativeUiTests|NativeEditor)(\.(exe|lib)\.recipe|\.Build\.CppClean\.log)$' -or
            $Relative -eq ('obj\' + $producer + '\' + $producer + '.tlog\unsuccessfulbuild')
    }
    # tmp is reserved at creation for this native runner's synthetic fixtures only.
    return $Relative.StartsWith('tmp\',[StringComparison]::OrdinalIgnoreCase)
}

function Seal-VerificationRun($Run) {
    $root = Resolve-VerificationRun $Run.Root
    Assert-NoArtifactProcess $root
    $inventory = Get-ArtifactInventory $root
    $entries = @($inventory.Files | ForEach-Object {
        $relative = $_.FullName.Substring($root.Length + 1)
        if (Test-VerificationOutput $relative) {
            [pscustomobject]@{Path=$relative;Bytes=$_.Length;Sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
        }
    })
    $Run.Files = $entries; $Run.State = 'sealed'; Write-VerificationManifest $Run
}

function Read-VerificationRun([string]$Path) {
    $root = Resolve-VerificationRun $Path
    $marker = Join-Path $root '.verification.json'
    $null = Assert-ArtifactPath $marker $root
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf) -or (Get-Item -LiteralPath $marker).Length -gt 4MB) { throw 'Missing or oversized ownership manifest.' }
    $run = [IO.File]::ReadAllText($marker) | ConvertFrom-Json
    if ($run.Owner -ne $script:verificationOwner -or $run.Root -ne $root -or
        $run.State -ne 'sealed' -or $run.Suite -notin @('Core','Ui') -or $run.Configuration -notin @('Debug','Release')) {
        throw "Unsealed or invalid verification ownership; retained: $root"
    }
    if (@($run.Files).Count -gt 20000) { throw 'Verification manifest exceeds the file-count bound.' }
    return $run
}

function Get-VerificationPlan([string]$Path) {
    $run = Read-VerificationRun $Path; $root = $run.Root
    $inventory = Get-ArtifactInventory $root
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $files = @(); $bytes = [long]0
    foreach ($entry in $run.Files) {
        if ([string]::IsNullOrWhiteSpace($entry.Path) -or $entry.Path.Length -gt 1024 -or
            [IO.Path]::IsPathRooted($entry.Path) -or $entry.Path -match '(^|[\\/])\.\.([\\/]|$)|:' -or
            -not (Test-VerificationOutput $entry.Path) -or $entry.Sha256 -notmatch '^[a-fA-F0-9]{64}$' -or
            -not $seen.Add($entry.Path)) { throw 'Invalid or duplicate manifest entry; nothing removed.' }
        $file = Assert-ArtifactPath (Join-Path $root $entry.Path) $root
        if (-not (Test-Path -LiteralPath $file)) { continue }
        $current = Get-Item -LiteralPath $file -Force
        if ($current.PSIsContainer -or $current.Length -ne $entry.Bytes -or (Get-FileHash -LiteralPath $file).Hash -ne $entry.Sha256) {
            throw "Registered output changed; nothing removed: $file"
        }
        $files += [pscustomobject]@{Path=$file;Bytes=$current.Length;Sha256=$entry.Sha256}; $bytes += $current.Length
    }
    $unknown = @($inventory.Files | Where-Object {
        $_.FullName -ne (Join-Path $root '.verification.json') -and -not $seen.Contains($_.FullName.Substring($root.Length + 1))
    })
    return [pscustomobject]@{Root=$root;Files=$files;Bytes=$bytes;Unknown=$unknown;Directories=$inventory.Directories}
}

function Clear-VerificationRun([string]$Path, [switch]$Apply) {
    $plan = Get-VerificationPlan $Path
    if (-not $Apply) { return $plan }
    Assert-NoArtifactProcess $plan.Root
    if ($plan.Unknown.Count) { throw "Unknown files retained in $($plan.Root); nothing removed." }
    $shellTarget = Get-ArtifactShellTarget
    if ($shellTarget -and @($plan.Files | Where-Object Path -EQ $shellTarget).Count) { throw 'An Explorer integration executable is protected.' }
    # Preflight every hash before the first deletion, then recheck each exact path.
    foreach ($file in $plan.Files) {
        $null = Assert-ArtifactPath $file.Path $plan.Root
        if ((Get-FileHash -LiteralPath $file.Path).Hash -ne $file.Sha256) { throw 'Output changed during cleanup; retained.' }
        Remove-Item -LiteralPath $file.Path -Force
    }
    $remaining = Get-ArtifactInventory $plan.Root
    if (@($remaining.Files | Where-Object FullName -NE (Join-Path $plan.Root '.verification.json')).Count) { throw 'New files appeared; run retained.' }
    Remove-Item -LiteralPath (Join-Path $plan.Root '.verification.json') -Force
    Remove-EmptyArtifactDirectories $remaining.Directories $plan.Root
    if (Test-Path -LiteralPath $plan.Root) { throw 'Verification directory remains; cleanup incomplete.' }
    return [pscustomobject]@{FilesRemoved=$plan.Files.Count;BytesRemoved=$plan.Bytes;RootRemoved=$true}
}

function Invoke-VerificationLifecycle {
    param([string]$Suite, [string]$Configuration, [scriptblock]$Action)
    $run = New-VerificationRun -Suite $Suite -Configuration $Configuration
    $result = [pscustomobject]@{Outcome='PASS';Error='';Cleanup='pending';Root=$run.Root;FilesRemoved=0;BytesRemoved=[long]0;
        RemainingFiles=0;RemainingBytes=[long]0;RetainedEntries=@();RemainingError='';Groups=@()}
    $oldTemp = $env:TEMP; $oldTmp = $env:TMP
    try {
        $env:TEMP = Join-Path $run.Root 'tmp'; $env:TMP = $env:TEMP
        & $Action $run $result | Out-Host
    } catch {
        $result.Outcome = if ($_.Exception -is [OperationCanceledException]) {'CANCELLED'} else {'FAIL'}
        $result.Error = $_.Exception.Message
    } finally {
        $env:TEMP = $oldTemp; $env:TMP = $oldTmp
        try {
            Seal-VerificationRun $run
            $cleaned = Clear-VerificationRun $run.Root -Apply
            $result.Cleanup = 'PASS'; $result.FilesRemoved = $cleaned.FilesRemoved; $result.BytesRemoved = $cleaned.BytesRemoved
        } catch {
            $result.Outcome = 'INCOMPLETE'; $result.Cleanup = $_.Exception.Message
            $result.FilesRemoved = $null; $result.BytesRemoved = $null
            try {
                $left = Get-ArtifactInventory $run.Root
                $result.RemainingFiles = $left.Files.Count
                $result.RemainingBytes = [long](($left.Files | Measure-Object Length -Sum).Sum)
                $result.RetainedEntries = @($left.Files | Select-Object -First 100 | ForEach-Object {
                    [pscustomobject]@{Path=$_.FullName.Substring($run.Root.Length + 1);Bytes=$_.Length}
                })
            } catch { $result.RemainingFiles = -1; $result.RemainingBytes = -1; $result.RemainingError = $_.Exception.Message }
        }
    }
    return $result
}
