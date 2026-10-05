param(
    [switch]$RemoveUserData
)

$ErrorActionPreference = "Stop"
$running = Get-Process -Name "MouseEngine.Host.Windows" -ErrorAction SilentlyContinue
if ($running) { throw "Mouse Engine is running. Close it before uninstalling." }

$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Mouse Engine"
if (Test-Path $installRoot) {
    Remove-Item $installRoot -Recurse -Force
}

$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\Mouse Engine"
if (Test-Path $startMenu) {
    Remove-Item $startMenu -Recurse -Force
}

if ($RemoveUserData) {
    $workspace = Join-Path $env:USERPROFILE "Documents\Mouse Engine"
    $cache = Join-Path $env:LOCALAPPDATA "Mouse Engine"
    if (Test-Path $workspace) { Remove-Item $workspace -Recurse -Force }
    if (Test-Path $cache) { Remove-Item $cache -Recurse -Force }
}

Write-Output "UNINSTALL=PASS"
