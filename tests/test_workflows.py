"""Regression checks for test and release workflow behavior."""

import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]


def load_workflow(name):
    return yaml.safe_load((ROOT / ".github/workflows" / name).read_text())


class WorkflowTests(unittest.TestCase):
    def test_zig_build_includes_all_c_sources_and_tests(self):
        build = (ROOT / "build.zig").read_text()
        listed = set(re.findall(r'"((?:src|tests|examples)/[^"\n]+\.c)"', build))
        expected = {
            path.relative_to(ROOT).as_posix()
            for path in [*(ROOT / "src").rglob("*.c"), *(ROOT / "tests").glob("*.c")]
        }
        expected.add("examples/hsdlib_example.c")
        self.assertEqual(listed, expected)

    def test_native_release_runners_match_artifact_architectures(self):
        matrix = load_workflow("publish_py.yml")["jobs"]["build-libraries"]["strategy"]["matrix"]
        runners = {
            "ubuntu-latest": "amd64",
            "ubuntu-24.04-arm": "arm64",
            "macos-15-intel": "amd64",
            "macos-latest": "arm64",
            "windows-latest": "amd64",
            "windows-11-arm": "arm64",
        }
        self.assertEqual(len(matrix["include"]), 6)
        for entry in matrix["include"]:
            with self.subTest(artifact=entry["artifact-name"]):
                self.assertEqual(runners[entry["os"]], entry["arch"])
                if entry["os"].startswith("windows"):
                    expected = "CLANGARM64" if entry["arch"] == "arm64" else "UCRT64"
                    self.assertEqual(entry["msystem"], expected)

    def test_wrong_binary_architecture_stops_artifact_upload(self):
        steps = load_workflow("publish_py.yml")["jobs"]["build-libraries"]["steps"]
        check = next(step["run"] for step in steps if "EXPECTED_ARCH" in step.get("env", {}))
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "library").write_bytes(b"library fixture")
            file_command = root / "file"
            file_command.write_text('#!/bin/sh\nprintf "%s\\n" "$FILE_DESCRIPTION"\n')
            file_command.chmod(0o755)
            for description, expected, succeeds in [
                ("ELF 64-bit LSB shared object, x86-64", "aarch64|arm64", False),
                ("ELF 64-bit LSB shared object, ARM aarch64", "aarch64|arm64", True),
                ("Mach-O 64-bit dynamically linked shared library arm64", "aarch64|arm64", True),
                ("PE32+ executable (DLL) Aarch64", "aarch64|arm64", True),
                ("PE32+ executable (DLL) x86-64", "x86-64|x86_64", True),
                ("ELF 64-bit LSB shared object, ARM aarch64", "x86-64|x86_64", False),
            ]:
                with self.subTest(description=description, expected=expected):
                    artifact = root / "artifact"
                    artifact.unlink(missing_ok=True)
                    env = dict(
                        os.environ,
                        PATH=f"{root}:{os.environ['PATH']}",
                        FILE_DESCRIPTION=description,
                        EXPECTED_ARCH=expected,
                        LIBRARY=str(root / "library"),
                        ARTIFACT=str(artifact),
                    )
                    result = subprocess.run(["bash", "-c", check], env=env, capture_output=True)
                    self.assertEqual(result.returncode == 0, succeeds, result.stderr)
                    self.assertEqual(artifact.exists(), succeeds)

    def test_python_setup_precedes_system_pip_bootstrap(self):
        bootstraps = 0
        for path in (ROOT / ".github/workflows").glob("*.yml"):
            for job, config in load_workflow(path.name)["jobs"].items():
                with self.subTest(workflow=path.name, job=job):
                    setup = False
                    for step in config["steps"]:
                        if step.get("uses", "").startswith("actions/setup-python@"):
                            setup = True
                        command = step.get("run", "")
                        if any(text in command for text in ["pip install", "make install-deps"]):
                            self.assertTrue(setup, "Python setup must precede pip installation")
                            bootstraps += 1
        self.assertGreater(bootstraps, 0)

    def test_wheel_tests_do_not_rebuild_the_release(self):
        steps = load_workflow("publish_py.yml")["jobs"]["build-wheel"]["steps"]
        command = next(
            step["run"]
            for step in steps
            if "tests/test_release_wheel.py" in step.get("run", "")
        )
        self.assertEqual(command.count("uv build"), 1)
        self.assertNotIn("make ", command)
        self.assertIn("uv pip install dist/*.whl", command)
        self.assertIn(".venv/bin/python -m pytest", command)
        self.assertLess(command.index("-m pytest"), command.index("tests/test_release_wheel.py"))


if __name__ == "__main__":
    unittest.main()
