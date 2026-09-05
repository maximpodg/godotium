"""Fetch the pinned upstream source archive."""
import ast
import json
from pathlib import Path
import sys
import tempfile
from patch_godot import apply_patches
from build_utils import download, extract, file_lock, pins, stamp


def source_version(source):
    # Parse data without executing an upstream Python file.
    values = {}
    for node in ast.parse((source / "version.py").read_text()).body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            if node.targets[0].id in ("major", "minor", "patch", "status"):
                values[node.targets[0].id] = ast.literal_eval(node.value)
    numbers = [str(values["major"]), str(values["minor"])]
    if values["patch"]:
        numbers.append(str(values["patch"]))
    return ".".join(numbers) + "-" + values["status"]


def validate_source(source, version):
    if not (source / "SConstruct").is_file():
        raise RuntimeError(f"{source} is not a Godot source tree")
    identity = {key: version[key] for key in
                ("GODOT_VERSION", "GODOT_SOURCE_URL", "GODOT_SOURCE_SHA256")}
    marker = source / ".godotium-source.json"
    if source_version(source) != version["GODOT_VERSION"] or (
            marker.exists() and json.loads(marker.read_text()) != identity):
        raise RuntimeError(
            f"Godot source pin mismatch in {source}. Move this cached directory "
            "aside and rebuild to fetch the pinned source; existing files were not replaced.")
    # Adopt legacy caches only after checking their declared engine version.
    # New downloads have already passed archive SHA-256 verification.
    if not marker.exists():
        marker.write_text(json.dumps(identity, indent=2) + "\n")


def main(version_file, source, output):
    source = Path(source).resolve()
    version = pins(version_file)
    with file_lock(source.parent / ".godot-fetch.lock"):
        if not source.exists():
            with tempfile.TemporaryDirectory(dir=source.parent, prefix="godot-download-") as temporary:
                archive = Path(temporary) / "godot.tar.xz"
                download(version["GODOT_SOURCE_URL"], version["GODOT_SOURCE_SHA256"], archive)
                extract(archive, temporary)
                (Path(temporary) / ("godot-" + version["GODOT_VERSION"])).rename(source)
        validate_source(source, version)
        apply_patches(source)
        stamp(output)


if __name__ == "__main__":
    main(*sys.argv[1:])
