"""Install checksum-verified GN and Ninja for this host (Python 3.11.8+)."""
import os
import platform
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
from build_utils import TOOLS, download, host_os

GN_REVISION = "3357c4f51b1a9e676378c695dd9c7e9911c35ee6"
HOSTS = {
    "mac": ("arm64", "mac-arm64", "9cf33cd2d2426bb4559ab46174ec1031eac9919ed24971174327b6ab4331e415",
            "89a287444b5b3e98f88a945afa50ce937b8ffd1dcc59c555ad9b1baf855298c9"),
    "win": ("x86_64", "windows-amd64", "77e77e2f0d7bea1992769343c68ab4312b8151c5a433f30301b365dd8e0f8687",
            "f550fec705b6d6ff58f2db3c374c2277a37691678d6aba463adcbb129108467a"),
    "linux": ("x86_64", "linux-amd64", "db0ef95a0cceda773650c5ede8f0ea9cadcb0aa6c767ed75304e5622be212504",
              "6f98805688d19672bd699fbbfa2c2cf0fc054ac3df1f0e6a47664d963d530255"),
}


def install(name, version, url, checksum):
    filename = name + (".exe" if os.name == "nt" else "")
    executable = TOOLS / "bin" / filename
    if executable.is_file():
        actual = subprocess.check_output([str(executable), "--version"], text=True).strip()
        if actual == version:
            return
    with tempfile.TemporaryDirectory(dir=TOOLS, prefix="download-") as temporary:
        archive = Path(temporary) / "tool.zip"
        print(f"Installing {name} {version}...", flush=True)
        download(url, checksum, archive)
        with zipfile.ZipFile(archive) as package:
            package.extract(filename, temporary)
        staged = Path(temporary) / filename
        staged.chmod(0o755)
        actual = subprocess.check_output([str(staged), "--version"], text=True).strip()
        if actual != version:
            raise RuntimeError(f"Unexpected {name} version: {actual}")
        staged.replace(executable)


def main():
    if sys.version_info < (3, 11, 8):
        raise RuntimeError("Python 3.11.8 or newer is required.")
    system = host_os()
    cpu, package, gn_sha, ninja_sha = HOSTS[system]
    machine = platform.machine().lower().replace("amd64", "x86_64").replace("aarch64", "arm64")
    if machine != cpu:
        raise RuntimeError("Supported hosts: Apple Silicon Mac, x64 Windows and x64 Linux.")
    (TOOLS / "bin").mkdir(parents=True, exist_ok=True)
    install("gn", "2407 (3357c4f51b1a)",
            f"https://chrome-infra-packages.appspot.com/dl/gn/gn/{package}/+/git_revision:{GN_REVISION}", gn_sha)
    install("ninja", "1.12.1",
            f"https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-{system}.zip", ninja_sha)
    # Use the actual interpreter: Ninja on Windows cannot directly execute a
    # .cmd shim. Forward slashes also keep the path usable in GN strings.
    (TOOLS / "python_path.txt").write_text(
        Path(sys.executable).resolve().as_posix() + "\n")


if __name__ == "__main__":
    main()
