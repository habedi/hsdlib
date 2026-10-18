"""Checks for the assembled release wheel, run from an environment where with the wheel installed in it."""

import re
import sys
import unittest
from pathlib import Path
from zipfile import ZipFile


class ReleaseWheelTests(unittest.TestCase):
    def test_platform_libraries_match_downloaded_artifacts(self):
        libraries = {
            "libhsd-amd64.so",
            "libhsd-arm64.so",
            "libhsd-amd64.dylib",
            "libhsd-arm64.dylib",
            "hsd-amd64.dll",
            "hsd-arm64.dll",
        }
        wheels = list(Path("dist").glob("*.whl"))
        self.assertEqual(len(wheels), 1, "Expected exactly one release wheel")
        with ZipFile(wheels[0]) as wheel:
            bundled = {
                Path(name).name
                for name in wheel.namelist()
                if name.endswith((".so", ".dylib", ".dll"))
            }
            self.assertEqual(bundled, libraries)
            for name in libraries:
                with self.subTest(library=name):
                    artifact = Path("temp-libs") / name / name
                    self.assertEqual(wheel.read(f"hsdpy/{name}"), artifact.read_bytes())

    def test_import_uses_installed_wheel_and_its_native_library(self):
        import hsdpy

        package = Path(hsdpy.__file__).resolve().parent
        header = Path("include/hsdlib.h").read_text()
        version = re.search(r'^#define HSD_VERSION "([^"]+)"$', header, re.MULTILINE)
        self.assertIsNotNone(version)
        self.assertEqual(hsdpy.__version__, version.group(1))
        self.assertTrue(package.is_relative_to(Path(sys.prefix).resolve()))
        library = Path(hsdpy.get_library_info()["lib_path"]).resolve()
        self.assertEqual(library.parent, package)
        self.assertTrue(library.is_file())


if __name__ == "__main__":
    unittest.main()
