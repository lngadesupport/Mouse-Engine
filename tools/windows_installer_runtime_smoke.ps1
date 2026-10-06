param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerPath
)

$ErrorActionPreference = "Stop"
$InstallerPath = (Resolve-Path $InstallerPath).Path
if (-not (Test-Path $InstallerPath)) { throw "Installer missing: $InstallerPath" }

$bytes = [System.IO.File]::ReadAllBytes($InstallerPath)
if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw "Installer is not a PE file"
}

$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Mouse Engine"
$installedHost = Join-Path $installRoot "MouseEngine.Host.Windows.exe"
$uninstaller = Join-Path $installRoot "unins000.exe"
$logPath = Join-Path $env:TEMP "MouseEngine-installer-smoke.log"

if (Test-Path $installRoot) {
    throw "Installer smoke requires a clean install root: $installRoot"
}

& $InstallerPath /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=$logPath
if ($LASTEXITCODE -ne 0) { throw "Installer exited with code $LASTEXITCODE" }
if (-not (Test-Path $installedHost)) { throw "Installed host missing: $installedHost" }
if (-not (Test-Path $uninstaller)) { throw "Uninstaller missing: $uninstaller" }

& $installedHost --self-test
if ($LASTEXITCODE -ne 0) { throw "Installed host self-test failed with code $LASTEXITCODE" }

& $uninstaller /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
if ($LASTEXITCODE -ne 0) { throw "Uninstaller exited with code $LASTEXITCODE" }

if (Test-Path $installRoot) {
    throw "Installer smoke cleanup failed; install root still exists: $installRoot"
}

Write-Output "WINDOWS_INSTALLER_RUNTIME_SMOKE=PASS"
