import re
from pathlib import Path

from setuptools import setup

header = (Path(__file__).parent / "include/hsdlib.h").read_text(encoding="utf-8")
version = re.search(r'^#define HSD_VERSION "([^"]+)"$', header, re.MULTILINE)
if version is None:
    raise RuntimeError("HSD_VERSION is missing from include/hsdlib.h")

setup(version=version.group(1))
