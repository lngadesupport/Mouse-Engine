from __future__ import annotations
import json, tempfile, zipfile
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / "packaging/RELEASE_MANIFEST.json").read_text(encoding="utf-8"))
    assert manifest["mutationDefault"] == "denied"
    assert manifest["architecture"] == "x64"
    assert manifest["workspace"] == "%USERPROFILE%\\Documents\\Mouse Engine"
    assert manifest["cache"] == "%LOCALAPPDATA%\\Mouse Engine"

    required = [
        root / "Install-MouseEngine.cmd",
        root / "Uninstall-MouseEngine.cmd",
        root / "packaging/install.ps1",
        root / "packaging/uninstall.ps1",
        root / "packaging/RELEASE_MANIFEST.json",
        root / "tools/windows_installer_runtime_smoke.ps1",
    ]
    for path in required:
        assert path.is_file(), path

    install_script = (root / "packaging/install.ps1").read_text(encoding="utf-8")
    uninstall_script = (root / "packaging/uninstall.ps1").read_text(encoding="utf-8")
    assert "WORKSPACE=$env:USERPROFILE\\Documents\\Mouse Engine" in install_script
    assert "CACHE=$env:LOCALAPPDATA\\Mouse Engine" in install_script
    assert 'Join-Path $env:USERPROFILE "Documents\\Mouse Engine"' in uninstall_script
    assert 'Join-Path $env:LOCALAPPDATA "Mouse Engine"' in uninstall_script

    self_test_marker = '& (Join-Path $installRoot "MouseEngine.Host.Windows.exe") --self-test'
    backup_cleanup_marker = 'if (Test-Path $backupRoot) { Remove-Item $backupRoot -Recurse -Force }'
    assert self_test_marker in install_script
    assert backup_cleanup_marker in install_script
    assert install_script.index(self_test_marker) < install_script.index(backup_cleanup_marker)
    assert 'Move-Item $backupRoot $installRoot -Force -ErrorAction SilentlyContinue' in install_script
    assert 'elseif (Test-Path $installRoot)' in install_script
\n    runtime_smoke = (root / "tools/windows_installer_runtime_smoke.ps1").read_text(encoding="utf-8")
    for marker in [
        "VERYSILENT",
        "--self-test",
        "unins000.exe",
        "Installer smoke cleanup failed",
        "WINDOWS_INSTALLER_RUNTIME_SMOKE=PASS",
    ]:
        assert marker in runtime_smoke, marker


    with tempfile.TemporaryDirectory() as td:
        fixture = Path(td) / "MouseEngine.Host.Windows.exe"
        fixture.write_bytes(b"MZ" + b"mouse-engine-test-fixture")
        archive = Path(td) / "fixture.zip"
        import subprocess, sys
        subprocess.run([
            sys.executable, str(root / "tools/package_windows.py"),
            "--source", str(root),
            "--host-executable", str(fixture),
            "--output", str(archive),
        ], check=True)
        with zipfile.ZipFile(archive) as zf:
            names = set(zf.namelist())
            assert "Mouse Engine/MouseEngine.Host.Windows.exe" in names
            assert "Mouse Engine/packaging/RELEASE_MANIFEST.json" in names
            assert "Mouse Engine/ui/index.html" in names

    print("package_windows_tests: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
