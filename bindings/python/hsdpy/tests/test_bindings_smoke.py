import os
import sys

# Ensure the local bindings/python directory is on sys.path so we import the package under development
here = os.path.dirname(__file__)
pkg_root = os.path.abspath(os.path.join(here, ".."))
if pkg_root not in sys.path:
    sys.path.insert(0, pkg_root)

import pytest
import hsdpy


def test_get_library_info_and_backend():
    # get_library_info should return a dict even if the shared lib couldn't be loaded
    info = hsdpy.get_library_info()
    assert isinstance(info, dict)
    assert 'lib_path' in info
    try:
        backend = hsdpy.get_backend()
    except Exception as e:
        pytest.skip(f"Could not load native library: {e}")
    else:
        assert isinstance(backend, str)
