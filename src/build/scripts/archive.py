"""Replace a native static archive so removed source files leave no old members."""
from pathlib import Path
import shlex
import subprocess
import sys

archiver, output, response_file = sys.argv[1:]
Path(output).unlink(missing_ok=True)
# Apple's ar does not expand @response files. GN emits slash-separated paths
# with shell quoting, so expand them here for all supported host archivers.
objects = shlex.split(Path(response_file).read_text())
subprocess.run([archiver, "rcs", output, *objects], check=True)
