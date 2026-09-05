"""Compile our C++ GDExtension without rebuilding the engine."""
from pathlib import Path
import json
import os
import subprocess
import sys
from build_utils import SRC_ROOT, file_lock, host_os, mingw_directory, venv_python


def main(output, jobs, target_os, target_cpu, mode, manifest, depfile, *static_libraries):
    module_files = [SRC_ROOT / path for path in json.loads(Path(manifest).read_text())]
    source_paths = sorted({path for path in module_files if path.suffix == ".cc"})
    if not source_paths:
        raise ValueError("The GN target must list its C++ sources.")
    output = Path(output).resolve()
    objects = output.parent / "obj"
    objects.mkdir(parents=True, exist_ok=True)
    system = {"mac": "macos", "win": "windows", "linux": "linux"}[target_os]
    common = [f"--jobs={jobs}", f"platform={system}",
              "arch=" + {"arm64": "arm64", "x64": "x86_64"}[target_cpu],
              f"target=template_{mode}", "lto=none", "use_static_cpp=yes"]

    def paths(src, out, obj):
        return ["-f", str(src / "build/scripts/game.SConstruct"),
                f"godotium_src={src}", f"godotium_output={out}",
                f"godotium_objects={obj}",
                "godotium_sources=" + json.dumps([str(src / path.relative_to(SRC_ROOT)) for path in source_paths]),
                "godotium_static_libraries=" + json.dumps([str(src / path) for path in static_libraries]),
                f"build_profile={src / 'build/config/cpp_profile.json'}"]

    with file_lock(output.parent / ".build.lock"):
        if target_os == "linux" and host_os() == "mac":
            image = (output.parent.parent / "templates/linux-image.id").read_text().strip()
            mounted_output = Path("/project") / output.relative_to(SRC_ROOT)
            command = ["docker", "run", "--rm", "--platform", "linux/arm64",
                       "--user", f"{os.getuid()}:{os.getgid()}",
                       "--mount", f"type=bind,source={SRC_ROOT},target=/project",
                       "--workdir", str(mounted_output.parent), image, "scons",
                       *paths(Path("/project"), mounted_output, mounted_output.parent / "obj"),
                       *common, "godotium_CC=x86_64-linux-gnu-gcc",
                       "godotium_CXX=x86_64-linux-gnu-g++", "godotium_LINK=x86_64-linux-gnu-g++"]
            subprocess.run(command, check=True)
        else:
            env = os.environ.copy()
            options = []
            if target_os == "win":
                mingw = mingw_directory()
                env["PATH"] = str(mingw / "bin") + os.pathsep + env["PATH"]
                options = ["use_mingw=yes", "use_llvm=yes", f"mingw_prefix={mingw}"]
            elif target_os == "mac":
                options = ["macos_deployment_target=11.0"]
            subprocess.run([str(venv_python(SRC_ROOT / ".tools/venv")), "-m", "SCons",
                            *paths(SRC_ROOT, output, objects), *common, *options],
                           cwd=output.parent, env=env, check=True)
        if not output.is_file():
            raise RuntimeError(f"Missing C++ library: {output}")
        output.touch()
        # Ninja tracks every declared module file, including headers, after the
        # first build. The GN-generated manifest tracks additions and removals.
        def escape(path):
            return os.path.relpath(path).replace("\\", "/").replace("$", "$$").replace(" ", "\\ ").replace("#", "\\#").replace(":", "\\:")

        dependencies = " ".join(escape(path) for path in sorted(set(module_files)))
        Path(depfile).parent.mkdir(parents=True, exist_ok=True)
        Path(depfile).write_text(f"{escape(output)}: {dependencies}\n")


if __name__ == "__main__":
    main(*sys.argv[1:])
