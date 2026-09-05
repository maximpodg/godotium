"""Build the Linux cross compiler container on Apple Silicon."""
from pathlib import Path
import shutil
import subprocess
import sys
from build_utils import SRC_ROOT

if not shutil.which("docker") or subprocess.run(
        ["docker", "info"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
    sys.exit("Linux cross builds need Docker. Start Docker Desktop or colima start --profile godotium; see README.md")
output = Path(sys.argv[1]).resolve()
output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(["docker", "build", "--platform", "linux/arm64", "--iidfile", str(output),
                str(SRC_ROOT / "build/docker")], check=True)
