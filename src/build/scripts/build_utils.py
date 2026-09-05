"""Shared, portable helpers for the pinned build tools."""
from contextlib import contextmanager
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import time
import zipfile

SRC_ROOT = Path(__file__).resolve().parents[2]
TOOLS = SRC_ROOT.parent / ".tools"


def host_os():
    return {"Darwin": "mac", "Windows": "win", "Linux": "linux"}[platform.system()]


def pins(path):
    return dict(line.split("=", 1) for line in Path(path).read_text().splitlines()
                if line and not line.startswith("#"))


def stamp(path, text=""):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def download(url, checksum, path):
    # curl uses the host certificate store, including on macOS Python installs.
    subprocess.run(["curl", "--fail", "--location", "--retry", "3", "--output",
                    str(path), url], check=True)
    with Path(path).open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != checksum:
        raise RuntimeError(f"Archive checksum mismatch: {url}")


def extract(archive, destination):
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as package:
            package.extractall(destination)
    else:
        with tarfile.open(archive) as package:
            package.extractall(destination, filter="data")


def venv_python(directory):
    return Path(directory) / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


def mingw_directory():
    version = pins(SRC_ROOT / "third_party/LLVM_MINGW_VERSION")["LLVM_MINGW_VERSION"]
    suffix = "x86_64" if host_os() == "win" else "macos-universal"
    return TOOLS / f"llvm-mingw-{version}-ucrt-{suffix}"


@contextmanager
def file_lock(path):
    # Lock one byte on Windows; POSIX uses flock. Closing releases either lock.
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as stream:
        if os.name == "nt":
            import msvcrt
            stream.seek(0, os.SEEK_END)
            if stream.tell() == 0:
                stream.write(b"0")
                stream.flush()
            while True:
                stream.seek(0)
                try:
                    msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError:
                    time.sleep(1)
        else:
            import fcntl
            fcntl.flock(stream, fcntl.LOCK_EX)
        yield
