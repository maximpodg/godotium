"""Install SCons into a project-local virtual environment."""
from pathlib import Path
import subprocess
import sys
import venv
from build_utils import file_lock, stamp, venv_python


def main(directory, output):
    directory = Path(directory).resolve()
    with file_lock(directory.parent / ".scons-setup.lock"):
        python = venv_python(directory)
        if not python.is_file():
            venv.EnvBuilder(with_pip=True).create(directory)
        check = subprocess.run([str(python), "-c",
                                "import SCons; assert SCons.__version__ == '4.8.1'"],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if check.returncode:
            subprocess.run([str(python), "-m", "pip", "install", "--disable-pip-version-check",
                            "SCons==4.8.1"], check=True)
        stamp(output)


if __name__ == "__main__":
    main(*sys.argv[1:])
