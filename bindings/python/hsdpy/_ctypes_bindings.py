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
    # Get system and architecture information
    system = platform.system()
    machine = platform.machine().lower()

    # Map architecture names to our standard names
    arch = "amd64" if machine in ("x86_64", "amd64") else "arm64" if machine in ("arm64", "aarch64") else machine

    # Set appropriate file extensions and prefixes based on OS
    lib_prefix = "lib"
    lib_suffix = ".so"
    if system == "Darwin":
        lib_suffix = ".dylib"
    elif system == "Windows":
        lib_prefix = ""
        lib_suffix = ".dll"

    # Create library names (arch-specific and generic)
    arch_lib_name = f"{lib_prefix}hsd-{arch}{lib_suffix}"
    generic_lib_name = f"{lib_prefix}hsd{lib_suffix}"

    _here = os.path.dirname(__file__)
    loaded_lib = None
    lib_info = {
        "system": system,
        "arch": arch,
        "lib_path": "Unknown"
    }

    # Try loading architecture-specific library first
    lib_path_arch = os.path.join(_here, arch_lib_name)
    if os.path.exists(lib_path_arch):
        try:
            print(f"Info: Found architecture-specific library: '{lib_path_arch}'", file=sys.stderr)
            loaded_lib = ctypes.CDLL(lib_path_arch) if system != "Windows" else ctypes.WinDLL(lib_path_arch)
            lib_info["lib_path"] = lib_path_arch
            return loaded_lib, lib_info
        except OSError as e:
            print(f"Warning: Found library at '{lib_path_arch}' but failed to load: {e}", file=sys.stderr)

    # Then try the generic library in the package
    lib_path_generic = os.path.join(_here, generic_lib_name)
    if os.path.exists(lib_path_generic):
        try:
            print(f"Info: Found generic library: '{lib_path_generic}'", file=sys.stderr)
            loaded_lib = ctypes.CDLL(lib_path_generic) if system != "Windows" else ctypes.WinDLL(lib_path_generic)
            lib_info["lib_path"] = lib_path_generic
            return loaded_lib, lib_info
        except OSError as e:
            print(f"Warning: Found library at '{lib_path_generic}' but failed to load: {e}", file=sys.stderr)

    print(f"Info: Library not found at expected package locations. Trying other methods...", file=sys.stderr)

    # Try environment variable
    lib_path_env = os.environ.get("HSDLIB_PATH")
    if lib_path_env:
        if os.path.exists(lib_path_env):
            try:
                loaded_lib = ctypes.CDLL(lib_path_env) if system != "Windows" else ctypes.WinDLL(lib_path_env)
                lib_info["lib_path"] = lib_path_env
                return loaded_lib, lib_info
            except OSError as e:
                raise ImportError(f"Failed to load library from HSDLIB_PATH '{lib_path_env}': {e}") from e
        else:
            print(f"Warning: HSDLIB_PATH '{lib_path_env}' not found.", file=sys.stderr)

    # Try various build paths
    project_root = os.path.join(_here, "..", "..")
    build_paths = [
        # Check for architecture-specific libraries first
        os.path.join(project_root, "lib", arch_lib_name),
        os.path.join(project_root, "build", arch_lib_name),
        os.path.join(project_root, "cmake-build-debug", arch_lib_name),
        os.path.join(project_root, arch_lib_name),
        os.path.join(_here, "..", arch_lib_name),

        # Then check for generic libraries
        os.path.join(project_root, "lib", generic_lib_name),
        os.path.join(project_root, "build", generic_lib_name),
        os.path.join(project_root, "cmake-build-debug", generic_lib_name),
        os.path.join(project_root, generic_lib_name),
        os.path.join(_here, "..", generic_lib_name),

        # Windows-specific paths
        os.path.join(project_root, "lib", f"hsd-{arch}.dll") if system == "Windows" else "",
        os.path.join(project_root, "build", f"hsd-{arch}.dll") if system == "Windows" else "",
        os.path.join(project_root, "cmake-build-debug", f"hsd-{arch}.dll") if system == "Windows" else "",
        os.path.join(project_root, "lib", "hsd.dll") if system == "Windows" else "",
        os.path.join(project_root, "build", "hsd.dll") if system == "Windows" else "",
        os.path.join(project_root, "cmake-build-debug", "hsd.dll") if system == "Windows" else "",
    ]

    for path in filter(None, build_paths):
        if os.path.exists(path):
            print(f"Info: Found library via development path: '{path}'", file=sys.stderr)
            try:
                loaded_lib = ctypes.CDLL(path) if system != "Windows" else ctypes.WinDLL(path)
                lib_info["lib_path"] = path
                return loaded_lib, lib_info
            except OSError as e:
                print(f"Warning: Found library at '{path}' but failed to load: {e}", file=sys.stderr)

    # Try find_library for both architecture-specific and generic names
    for lib_name in [f"hsd-{arch}", "hsd"]:
        found_path = find_library(lib_name)
        if found_path:
            print(f"Info: Found library via find_library: '{found_path}'", file=sys.stderr)
            try:
                loaded_lib = ctypes.CDLL(found_path) if system != "Windows" else ctypes.WinDLL(found_path)
                lib_info["lib_path"] = found_path
                return loaded_lib, lib_info
            except OSError as e:
                print(f"Warning: Found library '{found_path}' via find_library but failed to load: {e}",
                      file=sys.stderr)

    # Last resort: try system default search
    print(f"Info: Trying system default search for architecture-specific or generic library...", file=sys.stderr)
    for lib_name in [arch_lib_name, generic_lib_name]:
        try:
            loaded_lib = ctypes.CDLL(lib_name) if system != "Windows" else ctypes.WinDLL(lib_name)
            lib_info["lib_path"] = lib_name  # This may not be a full path
            return loaded_lib, lib_info
        except OSError:
            pass

    # Windows-specific fallbacks
    if system == "Windows":
        for dll_name in [f"hsd-{arch}.dll", "hsd.dll"]:
            try:
                loaded_lib = ctypes.WinDLL(dll_name)
                lib_info["lib_path"] = dll_name  # This may not be a full path
                return loaded_lib, lib_info
            except OSError:
                pass

    raise OSError(
        f"Could not load hsdlib. Searched for both architecture-specific ('{arch_lib_name}') and "
        f"generic ('{generic_lib_name}') libraries in package dir, HSDLIB_PATH, common build directories, "
        "and system paths. Please ensure the library is compiled and accessible."
    )


_lib, _lib_info = _load_hsd_library()


def get_library_info():
    """
    Return information about the currently loaded HSD library.

    Returns:
        dict: Dictionary containing library information including path,
              architecture, and loading details.
    """
    info = _lib_info.copy()

    # Add backend information if available
    if hsd_get_backend:
        try:
            backend_bytes = hsd_get_backend()
            if backend_bytes:
                info["backend"] = backend_bytes.decode('utf-8')
            else:
                info["backend"] = "unknown"
        except Exception:
            info["backend"] = "error retrieving backend info"
    else:
        info["backend"] = "function not available"

    return info


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
