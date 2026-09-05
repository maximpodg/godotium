"""Install a built host extension in the ignored source-project bin/ directory."""
from pathlib import Path
import sys
from build_utils import SRC_ROOT, host_os
from build_godot import copy_output


def main(build_dir):
    build_dir = Path(build_dir).resolve()
    filename = {"mac": "libgodotium_mac.dylib", "win": "libgodotium_win.dll",
                "linux": "libgodotium_linux.so"}[host_os()]
    candidates = [build_dir / folder / filename for folder in ("host_lib", "game_lib")]
    library = next((path for path in candidates if path.is_file()), None)
    if library is None:
        raise RuntimeError("Build game_cpp_host before preparing the source editor project.")
    output = SRC_ROOT / "bin" / filename
    output.parent.mkdir(parents=True, exist_ok=True)
    copy_output(library, output)
    print(f"Editor extension ready: {output}")


if __name__ == "__main__":
    main(*sys.argv[1:])
