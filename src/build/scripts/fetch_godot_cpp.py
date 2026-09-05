"""Fetch the pinned official C++ bindings without modifying the engine."""
from pathlib import Path
import sys
import tempfile
from build_utils import SRC_ROOT, download, extract, file_lock, pins, stamp


def main(output):
    values = pins(SRC_ROOT / "third_party/GODOT_CPP_VERSION")
    source = SRC_ROOT / "third_party/godot-cpp"
    revision = values["GODOT_CPP_REVISION"]
    with file_lock(source.parent / ".godot-cpp-fetch.lock"):
        if not source.exists():
            with tempfile.TemporaryDirectory(dir=source.parent, prefix="cpp-download-") as temporary:
                archive = Path(temporary) / "cpp.tar.gz"
                download(values["GODOT_CPP_URL"], values["GODOT_CPP_SHA256"], archive)
                extract(archive, temporary)
                extracted = Path(temporary) / ("godot-cpp-" + revision)
                (extracted / ".godotium-revision").write_text(revision)
                extracted.rename(source)
        if not (source / "SConstruct").is_file() or (source / ".godotium-revision").read_text() != revision:
            raise RuntimeError("godot-cpp version changed; remove third_party/godot-cpp and rebuild.")
        # The pinned bindings rely on an indirect stdlib include which is not
        # present with LLVM-MinGW 23. Keep this small source fix reproducible.
        implementation = source / "src/godot.cpp"
        text = implementation.read_text()
        if "#include <cstdlib>" not in text:
            anchor = '#include <godot_cpp/godot.hpp>'
            if anchor not in text:
                raise RuntimeError("godot-cpp stdlib patch no longer applies")
            implementation.write_text(text.replace(anchor, anchor + "\n\n#include <cstdlib>", 1))
        stamp(output)


if __name__ == "__main__":
    main(*sys.argv[1:])
