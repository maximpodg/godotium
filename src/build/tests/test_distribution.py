#!/usr/bin/env python3
"""Validate dist/ files and packaged UI; run Mac/Linux games away from sources."""

import argparse
import os
import platform
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

from check_windows_icon import check_windows_icon


def run(command, cwd):
    env = dict(os.environ, GODOTIUM_SETTINGS_PATH=str(Path(cwd) / "settings.cfg"))
    result = subprocess.run(command, cwd=cwd, env=env, text=True, encoding="utf-8", errors="replace", stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=120)
    print(result.stdout, end="", flush=True)
    if result.returncode or "ERROR:" in result.stdout or "SCRIPT ERROR:" in result.stdout:
        raise RuntimeError(f"Distribution check failed: {command}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("target_os", choices=("mac", "win", "linux"))
    args = parser.parse_args()
    build_dir = args.build_dir.resolve()
    editor = build_dir / ("bin/godot.exe" if platform.system() == "Windows" else "bin/godot")
    dist = build_dir / "dist"
    filename = {"mac": "Godotium.app", "win": "godotium.exe", "linux": "godotium"}[args.target_os]
    library_names = {"mac": "libgodotium_mac.dylib", "win": "libgodotium_win.dll",
                     "linux": "libgodotium_linux.so"}
    expected = {filename} if args.target_os == "mac" else {filename, "godotium.pck", library_names[args.target_os], "licenses"}
    if args.target_os == "linux":
        expected |= {"godotium.desktop", "godotium.svg"}
    if {p.name for p in dist.iterdir() if p.name != ".DS_Store"} != expected:
        raise RuntimeError("dist/ must contain only the complete distribution, without stale files.")
    notices = dist / "Godotium.app/Contents/Resources/licenses" if args.target_os == "mac" else dist / "licenses"
    for name in ("GODOTIUM_LICENSE.txt", "GODOT_LICENSE.txt", "GODOT_COPYRIGHT.txt", "GODOT_CPP_LICENSE.txt", "CC0-1.0.txt", "MUSIC_CREDITS.txt"):
        if not (notices / name).is_file() or (notices / name).stat().st_size < 100:
            raise RuntimeError(f"Missing or empty distribution notice: {name}")
    script = Path(__file__).with_name("main_smoke.gd").resolve()
    with tempfile.TemporaryDirectory(prefix="godotium-dist-test-") as directory:
        directory = Path(directory).resolve()
        copied = directory / "dist"
        shutil.copytree(dist, copied)
        shutil.copy2(script, directory / script.name)
        executable = copied / filename
        pack = copied / "godotium.pck"
        if args.target_os == "mac":
            run(["codesign", "--verify", "--deep", "--strict", str(executable)], directory)
            signature = run(["codesign", "--display", "--verbose=4", str(executable)], directory)
            if "Signature=adhoc" not in signature:
                raise RuntimeError("Expected an ad-hoc signed Mac application.")
            library = executable / "Contents/Frameworks" / library_names["mac"]
            if not library.is_file():
                raise RuntimeError("Missing native gameplay library in Mac bundle.")
            run(["codesign", "--verify", "--strict", str(library)], directory)
            pack = executable / "Contents/Resources/Godotium.pck"
            executable /= "Contents/MacOS/Godotium"
            run([str(executable), "--headless", "--quit-after", "3"], directory)
        elif args.target_os == "win":
            check_windows_icon(executable, Path(__file__).resolve().parents[2] / "godotium/resources/icons/godotium.ico")
            with executable.open("rb") as stream:
                if stream.read(2) != b"MZ":
                    raise RuntimeError("Missing Windows executable header.")
                stream.seek(0x3C)
                stream.seek(struct.unpack("<I", stream.read(4))[0])
                if stream.read(6) != b"PE\0\0\x64\x86":
                    raise RuntimeError("Expected Windows x86_64 PE executable.")
            if platform.system() == "Windows":
                run([str(executable), "--headless", "--quit-after", "3"], directory)
        else:
            if (copied / "godotium.svg").read_bytes() != (Path(__file__).resolve().parents[2] / "godotium/resources/icons/godotium.svg").read_bytes():
                raise RuntimeError("Linux distribution icon differs from the game icon")
            with executable.open("rb") as stream:
                header = stream.read(20)
            if header[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", header, 18)[0] != 62:
                raise RuntimeError("Expected Linux x86_64 ELF executable.")
            if not executable.stat().st_mode & 0o111:
                raise RuntimeError("Linux executable lost its executable permission.")
            if platform.system() == "Linux":
                run([str(executable), "--headless", "--quit-after", "3"], directory)
            else:
                # Copy through the Docker API: macOS temporary folders are not
                # necessarily shared with the VM. The container gets only dist/.
                container = subprocess.check_output([
                    "docker", "create", "--platform", "linux/amd64",
                    "--env", "GODOT_SILENCE_ROOT_WARNING=1",
                    "ubuntu:22.04@sha256:2edbbc5dc405e9612ba3584ce95480277e3eb374407b5505fe26f17df77c7dbc",
                    "/game/godotium", "--headless", "--quit-after", "3",
                ], text=True).strip()
                try:
                    run(["docker", "cp", str(copied), f"{container}:/game"], directory)
                    run(["docker", "start", "--attach", container], directory)
                    status = subprocess.check_output(["docker", "wait", container], text=True).strip()
                    if status != "0":
                        raise RuntimeError(f"Linux runtime exited with status {status}")
                finally:
                    subprocess.run(["docker", "rm", "--force", container],
                                   stdout=subprocess.DEVNULL, check=True)
        with pack.open("rb") as stream:
            if stream.read(4) != b"GDPC":
                raise RuntimeError("Missing Godot resource pack header.")
        # Release templates disable script overrides. Drive actual GUI input
        # using the host editor with this target's exported PCK instead.
        # The host editor loads the exported scene's C++ class too. Put the
        # matching host library beside the test PCK's resource root. This also
        # permits testing a cross-exported pack without loading a foreign DLL.
        host = {"Darwin": "mac", "Windows": "win", "Linux": "linux"}[platform.system()]
        host_name = library_names[host]
        candidates = [build_dir / folder / host_name for folder in ("host_lib", "game_lib")]
        host_library = next(path for path in candidates if path.is_file())
        (pack.parent / "bin").mkdir(exist_ok=True)
        shutil.copy2(host_library, pack.parent / "bin" / host_name)
        ui = [str(editor), "--headless", "--main-pack", str(pack),
              "--path", str(pack.parent), "--script", str(directory / script.name)]
        # An unsupported saved locale must fall back to English. The second
        # process then verifies persistence of Russian selected by the first.
        (directory / "settings.cfg").write_text('[interface]\nlanguage="zz"\n')
        run(ui, directory)
        run(ui + ["--", "--mouse"], directory)
        contract = Path(__file__).with_name("scene_contract_smoke.gd").resolve()
        shutil.copy2(contract, directory / contract.name)
        previous_save_dir = os.environ.get("GODOTIUM_SAVE_DIR")
        os.environ["GODOTIUM_SAVE_DIR"] = str(directory / "contract-saves")
        try:
            run([str(editor), "--headless", "--main-pack", str(pack),
                 "--path", str(pack.parent), "--script", str(directory / contract.name)], directory)
        finally:
            if previous_save_dir is None:
                os.environ.pop("GODOTIUM_SAVE_DIR", None)
            else:
                os.environ["GODOTIUM_SAVE_DIR"] = previous_save_dir
    print(f"PASS: {args.target_os} distribution, native C++ main scene, settings/persistence, music, audio buses, licenses, keyboard and mouse exit.")
    if args.target_os == "win" and platform.system() != "Windows":
        print("Windows EXE execution is not tested on the Mac; test it on Windows before release.")


if __name__ == "__main__":
    main()
