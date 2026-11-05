[CmdletBinding()]
param([switch]$Apply)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'verification_artifacts.ps1')
$null = Assert-ArtifactPath $script:verificationBase $script:verificationBase
if (-not (Test-Path -LiteralPath $script:verificationBase)) { Write-Output 'No registered verification runs.'; exit 0 }
$runs = @(Get-ChildItem -LiteralPath $script:verificationBase -Directory -Force)
$plans = @()
foreach ($run in $runs) {
    # Validate the entire batch before applying any deletion.
    $plans += Get-VerificationPlan $run.FullName
}
if ($Apply -and @($plans | Where-Object {$_.Unknown.Count}).Count) { throw 'Unknown files found during batch preflight; nothing removed.' }
foreach ($plan in $plans) {
    Write-Output ("{0}: {1} files / {2} bytes; {3} unknown files retained: {4}" -f
        $(if ($Apply) {'Apply'} else {'Preview'}),$plan.Files.Count,$plan.Bytes,$plan.Unknown.Count,$plan.Root)
    if ($Apply) { Clear-VerificationRun $plan.Root -Apply | Out-Host }
    else {
        $plan.Files | ForEach-Object { Write-Output ('REMOVE ' + $_.Path) }
        $plan.Unknown | ForEach-Object { Write-Output ('KEEP unknown (' + $_.Length + ' bytes): ' + $_.FullName) }
    }
}
if (-not $Apply) { Write-Output 'Preview only. Use /apply to remove registered, hash-matched outputs.' }
