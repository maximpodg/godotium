"""Install the same LLVM-MinGW release on Mac and Windows."""
from pathlib import Path
import subprocess
import sys
import tempfile
from build_utils import SRC_ROOT, TOOLS, download, extract, file_lock, host_os, mingw_directory, pins, stamp


def main(output):
    values = pins(SRC_ROOT / "third_party/LLVM_MINGW_VERSION")
    directory = mingw_directory()
    windows = host_os() == "win"
    compiler = directory / "bin" / ("x86_64-w64-mingw32-clang++.exe" if windows else "x86_64-w64-mingw32-clang++")
    with file_lock(TOOLS / ".mingw-setup.lock"):
        if not compiler.is_file():
            with tempfile.TemporaryDirectory(dir=TOOLS, prefix="mingw-download-") as temporary:
                archive = Path(temporary) / ("mingw.zip" if windows else "mingw.tar.xz")
                key = "LLVM_MINGW_WINDOWS" if windows else "LLVM_MINGW"
                download(values[key + "_URL"], values[key + "_SHA256"], archive)
                extract(archive, temporary)
                (Path(temporary) / directory.name).rename(directory)
        subprocess.run([str(compiler), "--version"], check=True)
        stamp(output, str(directory) + "\n")


if __name__ == "__main__":
    main(*sys.argv[1:])
