#!/usr/bin/env python3
"""Export only distributable game files into a build directory's dist/ folder."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

PLATFORMS = {
    "mac": ("macOS", "Godotium.app"),
    "win": ("Windows Desktop", "godotium.exe"),
    "linux": ("Linux", "godotium"),
}


def stage_assets(project, assets, destination):
    asset_set = set(assets)
    for asset in assets:
        # If the editor has import settings, silently discarding them would
        # produce different results in the exported game.
        sidecar = Path(str(asset) + ".import")
        if sidecar.is_file() and sidecar not in asset_set:
            raise ValueError(f"List the import settings in export/BUILD.gn: {sidecar}")
        relative = asset.relative_to(project)
        if relative.suffix in (".cc", ".h", ".gn", ".gni"):
            raise ValueError(f"Build/code file listed as a game resource: {relative}")
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(asset, target)


def main():
    editor, template, project, dist, preset = (Path(p).absolute() for p in sys.argv[1:6])
    target_os, mode = sys.argv[6:8]
    game_library, host_library = (Path(p).resolve() for p in sys.argv[8:10])
    assets = [Path(path).resolve() for path in sys.argv[10:]]
    if project / "project.godot" not in assets:
        raise ValueError("GN must supply the explicit Godot resource list.")
    if mode not in ("debug", "release"):
        raise ValueError(f"Unsupported export mode: {mode}")
    platform_name, filename = PLATFORMS[target_os]
    dist.parent.mkdir(parents=True, exist_ok=True)
    # A fresh project copy prevents stale imports or deleted resources from
    # surviving a rebuild and lets out/Mac and out/Win export concurrently.
    with tempfile.TemporaryDirectory(prefix=".godotium-export-", dir=dist.parent) as work:
        work = Path(work)
        staged_project = work / "game"
        # Do not copy src/ recursively: it also contains out/, tools and C++.
        # This same GN list controls dependency tracking and staged resources.
        staged_project.mkdir()
        stage_assets(project, assets, staged_project)
        (staged_project / "bin").mkdir()
        for library in {game_library, host_library}:
            shutil.copy2(library, staged_project / "bin" / library.name)
        settings = preset.read_text().replace("@TEMPLATE_PATH@", json.dumps(str(template)))
        (staged_project / "export_presets.cfg").write_text(settings)
        subprocess.run([str(editor), "--headless", "--editor", "--path",
                        str(staged_project), "--import"], check=True)
        staged_dist = work / "dist"
        staged_dist.mkdir()
        output = staged_dist / filename
        subprocess.run([str(editor), "--headless", "--path", str(staged_project),
                        f"--export-{mode}", platform_name, str(output)], check=True)
        notices = output / "Contents/Resources/licenses" if target_os == "mac" else staged_dist / "licenses"
        shutil.copytree(project.parent / "licenses", notices)
        shutil.copy2(project.parent / "LICENSE", notices / "GODOTIUM_LICENSE.txt")
        third_party = project / "third_party"
        for source, name in (
            (third_party / "godot-src/LICENSE.txt", "GODOT_LICENSE.txt"),
            (third_party / "godot-src/COPYRIGHT.txt", "GODOT_COPYRIGHT.txt"),
            (third_party / "godot-cpp/LICENSE.md", "GODOT_CPP_LICENSE.txt"),
        ):
            shutil.copy2(source, notices / name)
        if target_os == "mac":
            # Adding resources invalidates the export signature. Sign the final
            # bundle again, retaining its existing runtime entitlements.
            subprocess.run(["codesign", "--force", "--sign", "-",
                            "--preserve-metadata=entitlements,flags,runtime", str(output)], check=True)
            required = [f"{filename}/Contents/{relative}" for relative in (
                "MacOS/Godotium", "Resources/Godotium.pck", "Resources/icon.icns",
                "Info.plist", "_CodeSignature/CodeResources")]
            required += [f"{filename}/Contents/Frameworks/{game_library.name}"]
            subprocess.run(["codesign", "--verify", "--deep", "--strict", str(output)], check=True)
        else:
            required = [filename, "godotium.pck", game_library.name]
            if target_os == "linux":
                output.chmod(0o755)
                for name in ("godotium.desktop",):
                    shutil.copy2(project / "build/linux" / name, staged_dist / name)
                    required.append(name)
                shutil.copy2(project / "godotium/resources/icons/godotium.svg", staged_dist / "godotium.svg")
                required.append("godotium.svg")
        for relative in required:
            if not (staged_dist / relative).is_file():
                raise RuntimeError(f"Missing distribution output: {relative}")
        # Replace the complete distribution, not just the executable: switching
        # platforms or removing a dependency must not leave old DLLs or packs.
        previous = work / "previous-dist"
        if dist.exists():
            dist.rename(previous)
        try:
            staged_dist.rename(dist)
        except OSError:
            if previous.exists():
                previous.rename(dist)
            raise
    print(f"Built {dist / filename} ({mode}, {target_os})")


if __name__ == "__main__":
    main()
