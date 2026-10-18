"""Build checks for Zig packages with optional test and example sources."""

import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ZIG = os.environ.get("ZIG", "zig")


class ZigPackageTests(unittest.TestCase):
    def test_optional_sources_and_cached_configuration(self):
        with tempfile.TemporaryDirectory(prefix="hsdlib-zig-package-") as tmp:
            package = Path(tmp)
            for name in ["build.zig", "build.zig.zon", "LICENSE", "README.md"]:
                shutil.copy2(ROOT / name, package / name)
            for name in ["include", "src"]:
                shutil.copytree(ROOT / name, package / name)

            def build(*args):
                result = subprocess.run(
                    [ZIG, "build", *args, "-Dcpu=baseline"],
                    cwd=package,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout

            for present in [(), ("tests",), ("examples",), ("tests", "examples"), ()]:
                with self.subTest(directories=present):
                    for directory in ["tests", "examples"]:
                        destination = package / directory
                        if directory in present and not destination.exists():
                            shutil.copytree(ROOT / directory, destination)
                        elif directory not in present and destination.exists():
                            shutil.rmtree(destination)
                    build()
                    self.assertEqual(
                        (package / "zig-out/include/hsdlib.h").read_bytes(),
                        (ROOT / "include/hsdlib.h").read_bytes(),
                    )
                    self.assertGreaterEqual(len(list((package / "zig-out/lib").iterdir())), 2)
                    help_text = build("--help")
                    for directory, step in [("tests", "test"), ("examples", "example")]:
                        available = re.search(rf"^\s+{step}\s", help_text, re.MULTILINE) is not None
                        self.assertEqual(available, directory in present, help_text)
                        if available:
                            build(step)


if __name__ == "__main__":
    unittest.main()
