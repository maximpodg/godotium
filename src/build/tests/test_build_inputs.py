"""Regression tests for source pins and reproducible asset staging."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from export_game import stage_assets
from fetch_godot import main as fetch_godot, validate_source


class BuildInputsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "godot"
        self.source.mkdir()
        (self.source / "SConstruct").write_text("# fixture\n")
        (self.source / "version.py").write_text('major=4\nminor=7\npatch=2\nstatus="stable"\n')
        self.pin = {"GODOT_VERSION": "4.7.2-stable", "GODOT_SOURCE_URL": "https://example.invalid/godot.tar.xz",
                    "GODOT_SOURCE_SHA256": "0" * 64}

    def test_matching_legacy_cache_is_adopted(self):
        validate_source(self.source, self.pin)
        self.assertEqual(json.loads((self.source / ".godotium-source.json").read_text()), self.pin)
        validate_source(self.source, self.pin)

    def test_changed_pin_fails_without_stamp_or_download(self):
        validate_source(self.source, self.pin)
        for key, value in [("GODOT_VERSION", "4.8-stable"), ("GODOT_SOURCE_SHA256", "1" * 64)]:
            with self.subTest(key=key):
                pin = dict(self.pin, **{key: value})
                version_file = self.root / "GODOT_VERSION"
                version_file.write_text("\n".join(f"{k}={v}" for k, v in pin.items()))
                output = self.root / "fetch.stamp"
                with patch("fetch_godot.download") as download, patch("fetch_godot.apply_patches") as patches:
                    with self.assertRaisesRegex(RuntimeError, "pin mismatch"):
                        fetch_godot(version_file, self.source, output)
                    download.assert_not_called()
                    patches.assert_not_called()
                self.assertFalse(output.exists())

    def test_wrong_legacy_version_is_not_adopted(self):
        (self.source / "version.py").write_text('major=4\nminor=5\npatch=0\nstatus="stable"\n')
        with self.assertRaisesRegex(RuntimeError, "pin mismatch"):
            validate_source(self.source, self.pin)
        self.assertFalse((self.source / ".godotium-source.json").exists())

    def test_staging_keeps_import_settings_and_uid(self):
        image = self.source / "image.svg"
        image.write_text('<svg xmlns="http://www.w3.org/2000/svg"/>')
        sidecar = self.source / "image.svg.import"
        sidecar.write_text('[remap]\nuid="uid://example"\n[params]\nsvg/scale=2.0\n')
        destination = self.root / "stage"
        stage_assets(self.source, [image, sidecar], destination)
        self.assertEqual((destination / sidecar.name).read_bytes(), sidecar.read_bytes())
        self.assertEqual((destination / image.name).read_bytes(), image.read_bytes())

    def test_staging_rejects_omitted_import_settings(self):
        image = self.source / "image.svg"
        image.write_text("svg fixture")
        (self.source / "image.svg.import").write_text("import fixture")
        with self.assertRaisesRegex(ValueError, "import settings"):
            stage_assets(self.source, [image], self.root / "stage")


if __name__ == "__main__":
    unittest.main()
