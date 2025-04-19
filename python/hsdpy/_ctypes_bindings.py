import ctypes
import os
import platform
import sys
from ctypes import (
    c_float, c_int, c_size_t, c_uint16, c_uint8, c_uint64,
    POINTER, c_char_p
)
from ctypes.util import find_library


HSD_SUCCESS = 0
HSD_ERR_NULL_PTR = -1
HSD_ERR_UNSUPPORTED = -2
HSD_ERR_INVALID_INPUT = -3
HSD_FAILURE = -99

def _load_hsd_library():
    lib_prefix = "lib"
    lib_suffix = ".so"
    if platform.system() == "Darwin":
        lib_suffix = ".dylib"
    elif platform.system() == "Windows":
        lib_prefix = ""
        lib_suffix = ".dll"

    lib_name = f"{lib_prefix}hsd{lib_suffix}"
    _here = os.path.dirname(__file__)

    lib_path_pkg = os.path.join(_here, lib_name)
    if os.path.exists(lib_path_pkg):
        try:
            return ctypes.CDLL(lib_path_pkg) if platform.system() != "Windows" else ctypes.WinDLL(
                lib_path_pkg)
        except OSError as e:
            print(f"Warning: Found library at '{lib_path_pkg}' but failed to load: {e}", file=sys.stderr)

    print(
        f"Info: Library not found at expected package location '{lib_path_pkg}'. Trying other methods...", file=sys.stderr)

    lib_path_env = os.environ.get("HSDLIB_PATH")
    if lib_path_env:
        if os.path.exists(lib_path_env):
            try:
                return ctypes.CDLL(
                    lib_path_env) if platform.system() != "Windows" else ctypes.WinDLL(lib_path_env)
            except OSError as e:
                raise ImportError(
                    f"Failed to load library from HSDLIB_PATH '{lib_path_env}': {e}") from e
        else:
            print(f"Warning: HSDLIB_PATH '{lib_path_env}' not found.", file=sys.stderr)

    project_root = os.path.join(_here, "..", "..")
    build_paths = [
        os.path.join(project_root, "lib", lib_name), # Check ./lib/ first
        os.path.join(project_root, "lib", "hsd.dll") if platform.system() == "Windows" else "",
        os.path.join(project_root, "build", lib_name),
        os.path.join(project_root, "cmake-build-debug", lib_name),
        os.path.join(project_root, lib_name),
        os.path.join(_here, "..", lib_name),
        os.path.join(project_root, "build", "hsd.dll") if platform.system() == "Windows" else "",
        os.path.join(project_root, "cmake-build-debug",
                     "hsd.dll") if platform.system() == "Windows" else "",
    ]
    for path in filter(None, build_paths):
        if os.path.exists(path):
            print(f"Info: Found library via development path: '{path}'", file=sys.stderr)
            try:
                return ctypes.CDLL(path) if platform.system() != "Windows" else ctypes.WinDLL(path)
            except OSError as e:
                print(f"Warning: Found library at '{path}' but failed to load: {e}", file=sys.stderr)

    found_path = find_library("hsd")
    if found_path:
        print(f"Info: Found library via find_library: '{found_path}'", file=sys.stderr)
        try:
            return ctypes.CDLL(found_path) if platform.system() != "Windows" else ctypes.WinDLL(
                found_path)
        except OSError as e:
            raise ImportError(
                f"Found library '{found_path}' via find_library but failed to load: {e}") from e

    print(f"Info: Trying system default search for '{lib_name}' or 'hsd.dll'...", file=sys.stderr)
    try:
        return ctypes.CDLL(lib_name) if platform.system() != "Windows" else ctypes.WinDLL("hsd.dll")
    except OSError as e:
        if platform.system() == "Windows":
            try:
                return ctypes.WinDLL(lib_name)
            except OSError:
                pass

        raise OSError(
            f"Could not load hsdlib ('{lib_name}' or 'hsd.dll'). Searched package dir, HSDLIB_PATH, common build directories, "
            "and system paths. Please ensure the library is compiled and accessible."
        ) from e


_lib = _load_hsd_library()

c_float_p = POINTER(c_float)
c_size_t = c_size_t
c_uint16_p = POINTER(c_uint16)
c_uint8_p = POINTER(c_uint8)
c_uint64_p = POINTER(c_uint64)

def _setup_signature(func_name, restype, argtypes):
    try:
        func = getattr(_lib, func_name)
        func.argtypes = argtypes
        func.restype = restype
        return func
    except AttributeError:
        print(f"Warning: C function '{func_name}' not found in library.", file=sys.stderr)
        return None

hsd_dist_sqeuclidean_f32 = _setup_signature("hsd_dist_sqeuclidean_f32", c_int,
                                            [c_float_p, c_float_p, c_size_t, c_float_p])
hsd_sim_cosine_f32 = _setup_signature("hsd_sim_cosine_f32", c_int,
                                      [c_float_p, c_float_p, c_size_t, c_float_p])
hsd_dist_manhattan_f32 = _setup_signature("hsd_dist_manhattan_f32", c_int,
                                          [c_float_p, c_float_p, c_size_t, c_float_p])
hsd_sim_dot_f32 = _setup_signature("hsd_sim_dot_f32", c_int,
                                   [c_float_p, c_float_p, c_size_t, c_float_p])
hsd_sim_jaccard_u16 = _setup_signature("hsd_sim_jaccard_u16", c_int,
                                       [c_uint16_p, c_uint16_p, c_size_t, c_float_p])
hsd_dist_hamming_u8 = _setup_signature("hsd_dist_hamming_u8", c_int,
                                       [c_uint8_p, c_uint8_p, c_size_t, c_uint64_p])
hsd_get_backend = _setup_signature("hsd_get_backend", c_char_p, [])
