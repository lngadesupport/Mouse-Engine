from __future__ import annotations
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "packaging/RELEASE_MANIFEST.json").read_text(encoding="utf-8"))

required = [
    ".github/workflows/windows-release-candidate.yml",
    "CMakeLists.txt",
    "src/MouseEngine.Host.Windows/CMakeLists.txt",
    "src/MouseEngine.Host.Windows/main.cpp",
    "packaging/RELEASE_MANIFEST.json",
    "packaging/install.ps1",
    "packaging/uninstall.ps1",
    "Install-MouseEngine.cmd",
    "Uninstall-MouseEngine.cmd",
    "tools/package_windows.py",
    "tools/package_windows_tests.py",
    "tools/windows_installer_smoke.ps1",
    "tools/windows_installer_runtime_smoke.ps1",
]
for rel in required:
    if not (root / rel).is_file():
        raise SystemExit(f"required_files=FAIL missing {rel}")

if manifest["mutationDefault"] != "denied":
    raise SystemExit("mutationDefault safety gate failed")
if manifest["architecture"] != "x64":
    raise SystemExit("architecture gate failed")

print("RELEASE_GATE=PASS")
print("required_files=PASS")
print("mutation_default=DENIED")
print("physical_windows_qa=EXTERNAL_VALIDATION_REQUIRED")
print("webview2_validation=EXTERNAL_VALIDATION_REQUIRED")
print("RELEASE_STATUS=RELEASE_CANDIDATE")
