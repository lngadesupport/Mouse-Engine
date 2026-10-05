param(
    [Parameter(Mandatory = $true)]
    [string]$DistributionRoot
)

$ErrorActionPreference = "Stop"
$manifestPath = Join-Path $DistributionRoot "packaging\RELEASE_MANIFEST.json"
$hostPath = Join-Path $DistributionRoot "MouseEngine.Host.Windows.exe"
if (-not (Test-Path $manifestPath)) { throw "manifest missing" }
if (-not (Test-Path $hostPath)) { throw "host missing" }

$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
if ($manifest.mutationDefault -ne "denied") { throw "mutation safety gate failed" }

$pe = [System.IO.File]::ReadAllBytes($hostPath)
if ($pe.Length -lt 2 -or $pe[0] -ne 0x4D -or $pe[1] -ne 0x5A) {
    throw "host is not a PE file"
}

Write-Output "installer_package_fixture: PASS"
