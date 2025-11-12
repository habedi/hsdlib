import ctypes
import logging
import os
import platform
import sys
from ctypes import (
    c_float, c_int, c_size_t, c_uint16, c_uint8, c_uint64,
    POINTER, c_char_p
)
from ctypes.util import find_library

logger = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stderr)
handler.setFormatter(logging.Formatter('hsdpy [%(levelname)s]: %(message)s'))
logger.addHandler(handler)

if os.environ.get('HSD_DEBUG'):
    logger.setLevel(logging.INFO)
    logger.info("HSD_DEBUG environment variable detected. Enabling INFO level logging for hsdpy.")
else:
    logger.setLevel(logging.WARNING)

HSD_SUCCESS = 0
HSD_ERR_NULL_PTR = -1
HSD_ERR_UNSUPPORTED = -2
HSD_ERR_INVALID_INPUT = -3
HSD_FAILURE = -99

c_float_p = POINTER(c_float)
c_uint16_p = POINTER(c_uint16)
c_uint8_p = POINTER(c_uint8)
c_uint64_p = POINTER(c_uint64)


class LibraryLoader:
    @staticmethod
    def get_library_naming():
        system = platform.system()
        machine = platform.machine().lower()

        if machine in ("x86_64", "amd64"):
            arch = "amd64"
        elif machine in ("arm64", "aarch64"):
            arch = "arm64"
        else:
            arch = machine

        lib_prefix = "lib"
        lib_suffix = ".so"
        if system == "Darwin":
            lib_suffix = ".dylib"
        elif system == "Windows":
            lib_prefix = ""
            lib_suffix = ".dll"

        arch_lib_name = f"{lib_prefix}hsd-{arch}{lib_suffix}"
        generic_lib_name = f"{lib_prefix}hsd{lib_suffix}"

        return system, arch, arch_lib_name, generic_lib_name

    @staticmethod
    def try_load_library(lib_path, system):
        try:
            if system == "Windows":
                lib = ctypes.WinDLL(lib_path)
            else:
                lib = ctypes.CDLL(lib_path)
            logger.info(f"Found library: '{lib_path}'")
            return lib
        except OSError as e:
            logger.warning(f"Found library at '{lib_path}' but failed to load: {e}")
            return None

    @classmethod
    def load_hsd_library(cls):
        system, arch, arch_lib_name, generic_lib_name = cls.get_library_naming()

        _here = os.path.dirname(__file__)
        lib_info = {
            "system": system,
            "arch": arch,
            "lib_path": "Unknown"
        }

        for name, lib_name in [("architecture-specific", arch_lib_name),
                               ("generic", generic_lib_name)]:
            lib_path = os.path.join(_here, lib_name)
            if os.path.exists(lib_path):
                logger.info(f"Attempting to load package-local library: {lib_path}")
                loaded_lib = cls.try_load_library(lib_path, system)
                if loaded_lib:
                    lib_info["lib_path"] = lib_path
                    return loaded_lib, lib_info

        logger.info("Library not found at expected package locations. Trying other methods...")

        lib_path_env = os.environ.get("HSDLIB_PATH")
        if lib_path_env:
            logger.info(f"Checking HSDLIB_PATH environment variable: {lib_path_env}")
            if os.path.exists(lib_path_env):
                loaded_lib = cls.try_load_library(lib_path_env, system)
                if loaded_lib:
                    lib_info["lib_path"] = lib_path_env
                    return loaded_lib, lib_info
                else:
                    raise ImportError(f"Failed to load library from HSDLIB_PATH '{lib_path_env}'")
            else:
                logger.warning(f"HSDLIB_PATH '{lib_path_env}' points to a non-existent file.")

        project_root = os.path.abspath(os.path.join(_here, "..", ".."))
        logger.info(f"Searching common build paths relative to project root: {project_root}")

        build_paths = []
        common_dirs = ["lib", "build", "cmake-build-debug", "",
                       ".."]

        for dir_name in common_dirs:
            build_paths.append(os.path.join(project_root, dir_name, arch_lib_name))
            build_paths.append(os.path.join(project_root, dir_name, generic_lib_name))
            if system == "Windows":
                build_paths.append(os.path.join(project_root, dir_name, f"hsd-{arch}.dll"))
                build_paths.append(os.path.join(project_root, dir_name, "hsd.dll"))

        for path in build_paths:
            abs_path = os.path.abspath(path)
            if os.path.exists(abs_path):
                logger.info(f"Attempting to load library from potential build path: {abs_path}")
                loaded_lib = cls.try_load_library(abs_path, system)
                if loaded_lib:
                    lib_info["lib_path"] = abs_path
                    return loaded_lib, lib_info

        logger.info("Attempting search using ctypes.util.find_library...")
        for lib_name in [f"hsd-{arch}", "hsd"]:
            found_path = find_library(lib_name)
            if found_path:
                logger.info(f"Found library via find_library('{lib_name}'): '{found_path}'")
                loaded_lib = cls.try_load_library(found_path, system)
                if loaded_lib:
                    lib_info["lib_path"] = found_path
                    return loaded_lib, lib_info

        logger.info("Attempting system default search for library name...")
        for lib_name in [arch_lib_name, generic_lib_name]:
            try:
                logger.info(f"Trying default load for: {lib_name}")
                if system == "Windows":
                    loaded_lib = ctypes.WinDLL(lib_name)
                else:
                    loaded_lib = ctypes.CDLL(lib_name)
                logger.info(f"Successfully loaded '{lib_name}' via default system search.")
                lib_info["lib_path"] = lib_name
                return loaded_lib, lib_info
            except OSError:
                logger.info(f"Default load failed for: {lib_name}")
                pass

        if system == "Windows":
            logger.info("Trying Windows-specific DLL names in default search...")
            for dll_name in [f"hsd-{arch}.dll", "hsd.dll"]:
                try:
                    logger.info(f"Trying default load for: {dll_name}")
                    loaded_lib = ctypes.WinDLL(dll_name)
                    logger.info(f"Successfully loaded '{dll_name}' via default system search.")
                    lib_info["lib_path"] = dll_name
                    return loaded_lib, lib_info
                except OSError:
                    logger.info(f"Default load failed for: {dll_name}")
                    pass

        raise OSError(
            f"Could not load hsdlib. Searched for both '{arch_lib_name}' and "
            f"'{generic_lib_name}' libraries in package dir ({_here}), HSDLIB_PATH, "
            f"common build directories (relative to {_here}/../..), and system paths."
        )


def _setup_signature(func_name, restype, argtypes):
    try:
        func = getattr(_lib, func_name)
        func.argtypes = argtypes
        func.restype = restype
        logger.info(f"Successfully configured signature for C function '{func_name}'")
        return func
    except AttributeError:
        logger.warning(
            f"C function '{func_name}' not found in library '{_lib_info.get('lib_path', 'unknown path')}'.")
        return None


try:
    _lib, _lib_info = LibraryLoader.load_hsd_library()
except OSError as e:
    logger.warning(f"Could not load native hsdlib during import: {e}")
    _lib = None
    # Use the same mapping as LibraryLoader.get_library_naming() for arch
    try:
        system, arch, arch_lib_name, generic_lib_name = LibraryLoader.get_library_naming()
    except Exception:
        system = platform.system()
        arch = platform.machine().lower()
    _lib_info = {
        "system": system,
        "arch": arch,
        "lib_path": "not found"
    }

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


def get_library_info():
    info = _lib_info.copy()

    if hsd_get_backend:
        try:
            backend_bytes = hsd_get_backend()
            if backend_bytes:
                info["backend"] = backend_bytes.decode('utf-8')
            else:
                info["backend"] = "unknown (C function returned null)"
            logger.info(f"Detected backend via hsd_get_backend: {info['backend']}")
        except Exception as e:
            logger.warning(f"Error calling hsd_get_backend: {e}")
            info["backend"] = "error retrieving backend info"
    else:
        info["backend"] = "function not available"
        logger.info("hsd_get_backend function not found in C library.")

    return info
