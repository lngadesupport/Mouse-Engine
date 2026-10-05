from __future__ import annotations
import json, tempfile, zipfile
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / "packaging/RELEASE_MANIFEST.json").read_text(encoding="utf-8"))
    assert manifest["mutationDefault"] == "denied"
    assert manifest["architecture"] == "x64"

    required = [
        root / "Install-MouseEngine.cmd",
        root / "Uninstall-MouseEngine.cmd",
        root / "packaging/install.ps1",
        root / "packaging/uninstall.ps1",
        root / "packaging/RELEASE_MANIFEST.json",
    ]
    for path in required:
        assert path.is_file(), path

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

    print("package_windows_tests: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
