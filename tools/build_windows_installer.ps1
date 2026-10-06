param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$HostExecutable = (Join-Path $SourceRoot "build-release\src\MouseEngine.Host.Windows\Release\MouseEngine.Host.Windows.exe")
)

$ErrorActionPreference = "Stop"
$SourceRoot = (Resolve-Path $SourceRoot).Path
$HostExecutable = (Resolve-Path $HostExecutable).Path
$manifest = Get-Content (Join-Path $SourceRoot "packaging\RELEASE_MANIFEST.json") -Raw | ConvertFrom-Json
if ($manifest.mutationDefault -ne "denied") { throw "Safety gate failed: mutationDefault must be denied." }

$iscc = (Get-Command iscc.exe -ErrorAction Stop).Source
$hostBytes = [System.IO.File]::ReadAllBytes($HostExecutable)
if ($hostBytes.Length -lt 2 -or $hostBytes[0] -ne 0x4D -or $hostBytes[1] -ne 0x5A) { throw "Host executable is not a PE file." }
$stagedHost = Join-Path $SourceRoot "MouseEngine.Host.Windows.exe"
$dist = Join-Path $SourceRoot "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$output = Join-Path $dist "MouseEngine-1.0.0-rc.3-win64-setup.exe"
if (Test-Path $output) { Remove-Item $output -Force }
Copy-Item $HostExecutable $stagedHost -Force
try {
    & $iscc (Join-Path $SourceRoot "packaging\MouseEngine.iss")
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed." }
} finally {
    Remove-Item $stagedHost -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path $output)) { throw "Installer output missing: $output" }
Get-FileHash $output -Algorithm SHA256 | Format-List
Write-Output "WINDOWS_INSTALLER_BUILD=PASS"
