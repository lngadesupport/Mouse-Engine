param(
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot
)

$ErrorActionPreference = "Stop"
$PackageRoot = (Resolve-Path $PackageRoot).Path
$ManifestPath = Join-Path $PackageRoot "packaging\RELEASE_MANIFEST.json"
$HostPath = Join-Path $PackageRoot "MouseEngine.Host.Windows.exe"
if (-not (Test-Path $ManifestPath)) { throw "Missing RELEASE_MANIFEST.json" }
if (-not (Test-Path $HostPath)) { throw "Missing MouseEngine.Host.Windows.exe" }

$manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
if ($manifest.mutationDefault -ne "denied") { throw "Safety gate rejected package: mutationDefault must be denied." }

$running = Get-Process -Name "MouseEngine.Host.Windows" -ErrorAction SilentlyContinue
if ($running) { throw "Mouse Engine is running. Close it before installing or updating." }

$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Mouse Engine"
$stageRoot = Join-Path $env:LOCALAPPDATA ("Mouse Engine.installing." + [guid]::NewGuid().ToString("N"))
$backupRoot = Join-Path $env:LOCALAPPDATA ("Mouse Engine.backup." + [guid]::NewGuid().ToString("N"))
$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\Mouse Engine"
$shortcutPath = Join-Path $startMenu "Mouse Engine.lnk"

New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
try {
    Copy-Item -Path (Join-Path $PackageRoot "*") -Destination $stageRoot -Recurse -Force

    if (Test-Path $installRoot) {
        Move-Item -Path $installRoot -Destination $backupRoot -Force
    }

    try {
        New-Item -ItemType Directory -Force -Path (Split-Path $installRoot) | Out-Null
        Move-Item -Path $stageRoot -Destination $installRoot -Force
    } catch {
        if (Test-Path $installRoot) { Remove-Item $installRoot -Recurse -Force }
        if (Test-Path $backupRoot) { Move-Item $backupRoot $installRoot -Force }
        throw
    }

    New-Item -ItemType Directory -Force -Path $startMenu | Out-Null
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $HostPath.Replace($PackageRoot, $installRoot)
    $shortcut.WorkingDirectory = $installRoot
    $shortcut.Save()

    & (Join-Path $installRoot "MouseEngine.Host.Windows.exe") --self-test
    if ($LASTEXITCODE -ne 0) { throw "Installed host self-test failed." }

    if (Test-Path $backupRoot) { Remove-Item $backupRoot -Recurse -Force }

    Write-Output "INSTALL=PASS"
    Write-Output "INSTALL_ROOT=$installRoot"
    Write-Output "WORKSPACE=$env:USERPROFILE\Documents\Mouse Engine"
    Write-Output "CACHE=$env:LOCALAPPDATA\Mouse Engine"
} catch {
    if (Test-Path $stageRoot) { Remove-Item $stageRoot -Recurse -Force -ErrorAction SilentlyContinue }

    if (Test-Path $backupRoot) {
        if (Test-Path $installRoot) { Remove-Item $installRoot -Recurse -Force -ErrorAction SilentlyContinue }
        Move-Item $backupRoot $installRoot -Force -ErrorAction SilentlyContinue
    } elseif (Test-Path $installRoot) {
        Remove-Item $installRoot -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (Test-Path $shortcutPath) {
        Remove-Item $shortcutPath -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $startMenu) {
        $remaining = Get-ChildItem $startMenu -Force -ErrorAction SilentlyContinue
        if (-not $remaining) { Remove-Item $startMenu -Force -ErrorAction SilentlyContinue }
    }

    throw
}
