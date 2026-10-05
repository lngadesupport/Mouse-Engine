from __future__ import annotations
import argparse, json, shutil, zipfile
from pathlib import Path

REQUIRED = [
    "MouseEngine.Host.Windows.exe",
    "Install-MouseEngine.cmd",
    "Uninstall-MouseEngine.cmd",
    "packaging/RELEASE_MANIFEST.json",
    "packaging/install.ps1",
    "packaging/uninstall.ps1",
]

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--host-executable", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    source = Path(args.source).resolve()
    host = Path(args.host_executable).resolve()
    output = Path(args.output).resolve()

    manifest = json.loads((source / "packaging/RELEASE_MANIFEST.json").read_text(encoding="utf-8"))
    if manifest["mutationDefault"] != "denied":
        raise SystemExit("mutationDefault must be denied")
    if not host.is_file():
        raise SystemExit(f"host executable missing: {host}")

    stage = source / ".package-staging" / "Mouse Engine"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)

    shutil.copy2(host, stage / "MouseEngine.Host.Windows.exe")
    for rel in REQUIRED[1:]:
        src = source / rel
        if not src.is_file():
            raise SystemExit(f"required package file missing: {rel}")
        dst = stage / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)

    ui = source / "ui"
    if ui.is_dir():
        shutil.copytree(ui, stage / "ui", dirs_exist_ok=True)

    docs = source / "docs"
    if docs.is_dir():
        shutil.copytree(docs, stage / "docs", dirs_exist_ok=True)

    if output.exists():
        output.unlink()
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                zf.write(path, path.relative_to(stage.parent))

    shutil.rmtree(stage.parent)
    print(f"PACKAGE=PASS {output}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
