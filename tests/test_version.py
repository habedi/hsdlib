"""Version consistency and Python packaging regression tests."""

import os
import re
import runpy
import shutil
import subprocess
import sys
import tarfile
import tempfile
import types
import unittest
from email.parser import BytesParser
from pathlib import Path
from unittest.mock import Mock, patch
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]


class VersionTests(unittest.TestCase):
    def setUp(self):
        self.header = (ROOT / "include/hsdlib.h").read_text()
        match = re.search(r'^#define HSD_VERSION "([^"]+)"$', self.header, re.MULTILINE)
        self.assertIsNotNone(match)
        self.version = match.group(1)

    def test_zig_version_matches_header(self):
        manifest = (ROOT / "build.zig.zon").read_text()
        version = re.search(r'\.version = "([^"]+)"', manifest)
        self.assertIsNotNone(version)
        self.assertEqual(version.group(1), self.version)

    def test_release_tag_matches_header(self):
        if os.environ.get("GITHUB_REF_TYPE") != "tag":
            self.skipTest("Not a release tag build")
        self.assertEqual(os.environ["GITHUB_REF_NAME"], f"v{self.version}")

    def test_setup_reads_header_and_rejects_missing_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shutil.copy2(ROOT / "setup.py", root / "setup.py")
            (root / "include").mkdir()
            header = root / "include/hsdlib.h"
            setuptools = types.ModuleType("setuptools")
            setuptools.setup = Mock()
            with patch.dict(sys.modules, {"setuptools": setuptools}):
                header.write_text('#define HSD_VERSION "1.2.3"\n')
                runpy.run_path(str(root / "setup.py"))
                setuptools.setup.assert_called_once_with(version="1.2.3")
                setuptools.setup.reset_mock()
                header.write_text("/* No version macro. */\n")
                with self.assertRaisesRegex(RuntimeError, "HSD_VERSION is missing"):
                    runpy.run_path(str(root / "setup.py"))
                setuptools.setup.assert_not_called()

    def test_source_distribution_rebuilds_wheel_with_header_version(self):
        with tempfile.TemporaryDirectory(prefix="hsdlib-version-") as tmp:
            project = Path(tmp) / "project"
            project.mkdir()
            for name in ["pyproject.toml", "setup.py", "MANIFEST.in", "README.md", "LICENSE"]:
                shutil.copy2(ROOT / name, project / name)
            (project / "include").mkdir()
            shutil.copy2(ROOT / "include/hsdlib.h", project / "include/hsdlib.h")
            shutil.copytree(
                ROOT / "bindings/python", project / "bindings/python",
                ignore=shutil.ignore_patterns("*.so", "*.dylib", "*.dll", "__pycache__"),
            )

            def uv(*args):
                result = subprocess.run(
                    ["uv", *args], cwd=project, capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

            uv("build", "--sdist", "--out-dir", "dist")
            sdist = next((project / "dist").glob("*.tar.gz"))
            with tarfile.open(sdist) as archive:
                header = archive.extractfile(f"hsdpy-{self.version}/include/hsdlib.h")
                self.assertIsNotNone(header)
                self.assertEqual(header.read().decode(), self.header)
                metadata = archive.extractfile(f"hsdpy-{self.version}/PKG-INFO")
                self.assertEqual(BytesParser().parsebytes(metadata.read())["Version"], self.version)
            uv("build", "--wheel", str(sdist), "--out-dir", "dist")
            with ZipFile(next((project / "dist").glob("*.whl"))) as wheel:
                metadata = next(name for name in wheel.namelist() if name.endswith("/METADATA"))
                self.assertEqual(
                    BytesParser().parsebytes(wheel.read(metadata))["Version"], self.version
                )

            # A header-only version bump must refresh editable installation metadata.
            uv("sync", "--no-install-package", "numpy", "--python", sys.executable)
            updated_version = f"{self.version}.post1"
            (project / "include/hsdlib.h").write_text(
                self.header.replace(f'"{self.version}"', f'"{updated_version}"')
            )
            uv("sync", "--no-install-package", "numpy", "--python", sys.executable)
            version = subprocess.check_output(
                [str(project / ".venv/bin/python"), "-c",
                 "import importlib.metadata; print(importlib.metadata.version('hsdpy'))"],
                text=True,
            ).strip()
            self.assertEqual(version, updated_version)


if __name__ == "__main__":
    unittest.main()
