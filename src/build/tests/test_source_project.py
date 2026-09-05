#!/usr/bin/env python3
"""Check editor imports and runtime behavior from source, including autoloads."""
from pathlib import Path
import platform
import os
import subprocess
import sys
import tempfile
from test_distribution import run


def main():
    build_dir = Path(sys.argv[1]).resolve()
    source = Path(__file__).resolve().parents[2]
    editor = build_dir / ("bin/godot.exe" if platform.system() == "Windows" else "bin/godot")
    prepare = source / "build/scripts/prepare_editor.py"
    subprocess.run([sys.executable, str(prepare), str(build_dir)], check=True)
    script = Path(__file__).with_name("main_smoke.gd").resolve()
    with tempfile.TemporaryDirectory(prefix="godotium-source-test-") as temporary:
        directory = Path(temporary)
        run([str(editor), "--headless", "--editor", "--path", str(source), "--import"], directory)
        # Nonfinite saved volumes must fall back to the channel defaults.
        (directory / "settings.cfg").write_text('[interface]\nlanguage="zz"\n[audio]\nmusic=nan\nsound=inf\n')
        command = [str(editor), "--headless", "--path", str(source), "--script", str(script), "--", "--source"]
        run(command, directory)
        run(command + ["--mouse"], directory)
        timer_script = Path(__file__).with_name("timer_smoke.gd").resolve()
        timer_command = [str(editor), "--headless", "--path", str(source),
                         "--script", str(timer_script)]
        previous_save_path = os.environ.get("GODOTIUM_SAVE_DIR")
        os.environ["GODOTIUM_SAVE_DIR"] = str(directory / "saves")
        try:
            contract_script = Path(__file__).with_name("scene_contract_smoke.gd").resolve()
            run([str(editor), "--headless", "--path", str(source),
                 "--script", str(contract_script)], directory)
            run(timer_command, directory)
            run(timer_command + ["--", "--reload"], directory)
        finally:
            if previous_save_path is None:
                os.environ.pop("GODOTIUM_SAVE_DIR", None)
            else:
                os.environ["GODOTIUM_SAVE_DIR"] = previous_save_path
    print("PASS: source project imports, settings before scene creation, localization and UI persistence.")


if __name__ == "__main__":
    main()
