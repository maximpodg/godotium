#!/usr/bin/env python3
"""Build the host editor or a platform-specific template from pinned sources."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

from build_utils import file_lock, host_os, mingw_directory


def copy_output(source, output):
    with tempfile.TemporaryDirectory(dir=output.parent, prefix=".godot-") as temporary:
        staged = Path(temporary) / output.name
        shutil.copy2(source, staged)
        staged.replace(output)
    output.touch()


def build(source, output, jobs, dev_build, scons, target, target_os, target_cpu):
    if target not in ("editor", "template_release", "template_debug"):
        raise ValueError(f"Unsupported Godot target: {target}")
    if (target_os, target_cpu) not in (("mac", "arm64"), ("win", "x64"), ("linux", "x64")):
        raise ValueError(f"Unsupported platform: {target_os}/{target_cpu}")
    if target == "editor" and target_os != host_os():
        raise ValueError("The export editor must match the host OS.")
    source, output, scons = (Path(path).absolute() for path in (source, output, scons))
    if not (source / "SConstruct").is_file():
        raise RuntimeError("Godot sources are missing. Build the godot_sources target first.")
    output.parent.mkdir(parents=True, exist_ok=True)
    # Ninja pools are per invocation. A file lock also serializes SCons when
    # separate Ninja processes build out/Mac and out/Win simultaneously.
    with file_lock(source / ".godotium-build.lock"):
        if target_os == "win" and target != "editor":
            # Brand only the runtime resource; the host editor retains Godot's icon.
            icon = Path(__file__).resolve().parents[2] / "godotium/resources/icons/godotium.ico"
            staged_icon = source / "platform/windows/godotium.ico"
            if not staged_icon.exists() or staged_icon.read_bytes() != icon.read_bytes():
                shutil.copy2(icon, staged_icon)
            resource = source / "platform/windows/godot_res_template.rc"
            original = "GODOT_ICON ICON platform/windows/godot.ico"
            replacement = "GODOT_ICON ICON platform/windows/godotium.ico"
            text = resource.read_text()
            if replacement not in text:
                if text.count(original) != 1:
                    raise RuntimeError("Pinned Windows icon resource no longer matches.")
                resource.write_text(text.replace(original, replacement))
        platform_name = {"mac": "macos", "win": "windows", "linux": "linuxbsd"}[target_os]
        arch = {"arm64": "arm64", "x64": "x86_64"}[target_cpu]
        common = [f"--jobs={jobs}", f"platform={platform_name}", f"arch={arch}",
                  f"target={target}", f"dev_build={dev_build}", "vulkan=no",
                  "opengl3=yes", "accesskit=no", "angle=no"]
        if target_os == "linux" and host_os() == "mac":
            image = (output.parent / "linux-image.id").read_text().strip()
            subprocess.run([
                "docker", "run", "--rm", "--platform", "linux/arm64",
                "--user", f"{os.getuid()}:{os.getgid()}",
                "--mount", f"type=bind,source={source},target=/godot",
                "--workdir", "/godot", image, "scons", *common,
                "CC=x86_64-linux-gnu-gcc", "CXX=x86_64-linux-gnu-g++",
                "wayland=no", "use_static_cpp=yes", "use_sowrap=yes",
            ], check=True)
        else:
            if not scons.is_file():
                raise RuntimeError("SCons is missing. Build the scons_tool target first.")
            env = os.environ.copy()
            cache = source / "bin/build-cache"
            (cache / "clang-modules").mkdir(parents=True, exist_ok=True)
            (cache / "tmp").mkdir(exist_ok=True)
            env.update(CLANG_MODULE_CACHE_PATH=str(cache / "clang-modules"), TMPDIR=str(cache / "tmp"))
            options = ["metal=yes", "generate_bundle=yes"]
            if target_os == "win":
                mingw = mingw_directory()
                options = ["use_mingw=yes", "use_llvm=yes", f"mingw_prefix={mingw}",
                           "use_static_cpp=yes", "d3d12=no", "dcomp=no", "winrt=no", "xaudio2=no"]
                env["PATH"] = str(mingw / "bin") + os.pathsep + env["PATH"]
            elif target_os == "linux":
                options = ["wayland=no", "use_static_cpp=yes", "use_sowrap=yes"]
            subprocess.run([str(scons), "-m", "SCons", *common, *options], cwd=source, env=env, check=True)
        suffix = ".dev" if dev_build == "yes" else ""
        if target == "editor":
            extension = ".llvm.exe" if target_os == "win" else ""
            binary = source / f"bin/godot.{platform_name}.editor{suffix}.{arch}{extension}"
            # Copy instead of symlinking: another output directory may rebuild
            # the same host editor with different flags later.
            copy_output(binary, output)
        elif target_os == "mac":
            # Upstream calls even a single-architecture bundle "universal".
            # Name our ARM64 template accurately so export feature selection
            # and GDExtension library selection agree with the actual binary.
            archive = source / f"bin/godot_macos{suffix.replace('.', '_')}.zip"
            with tempfile.TemporaryDirectory(dir=output.parent) as temporary:
                staged = Path(temporary) / "macos.zip"
                with zipfile.ZipFile(archive) as original, zipfile.ZipFile(staged, "w") as result:
                    for entry in original.infolist():
                        data = original.read(entry.filename)
                        if "/Contents/MacOS/godot_macos_" in entry.filename:
                            entry.filename = entry.filename.replace(".universal", ".arm64")
                        result.writestr(entry, data)
                copy_output(staged, output)
        else:
            extension = ".llvm.exe" if target_os == "win" else ""
            copy_output(source / f"bin/godot.{platform_name}.{target}{suffix}.{arch}{extension}", output)
        # SCons can reuse an older binary. Mark this action's output current so
        # newer script inputs don't cause perpetual Ninja rebuilds.
        output.touch()


if __name__ == "__main__":
    build(*sys.argv[1:])
