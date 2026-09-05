"""Run a GN compiler/linker step in the same container used by Linux SCons."""
import os
from pathlib import Path
import subprocess
import sys

from build_utils import SRC_ROOT

build = Path.cwd()
image = (build / "templates/linux-image.id").read_text().strip()
subprocess.run([
    "docker", "run", "--rm", "--platform", "linux/arm64",
    "--user", f"{os.getuid()}:{os.getgid()}",
    "--mount", f"type=bind,source={SRC_ROOT},target=/project",
    "--workdir", str(Path("/project") / build.relative_to(SRC_ROOT)),
    image, *sys.argv[1:],
], check=True)
