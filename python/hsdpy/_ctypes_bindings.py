import os
import ctypes
from ctypes import c_float, c_int, c_size_t, c_uint16, c_int8, POINTER, byref
import numpy as np

# Load shared library
_here = os.path.dirname(__file__)
_lib = None
for name in ("libhsd.so", "libhsd.dylib", "hsd.dll"):
    path = os.path.join(_here, name)
    if os.path.exists(path):
        _lib = ctypes.CDLL(path)
        break
if _lib is None:
    from ctypes.util import find_library
    libname = find_library("hsd")
    if libname:
        _lib = ctypes.CDLL(libname)
if _lib is None:
    raise OSError("could not load hsdlib (libhsd.so / libhsd.dylib / hsd.dll)")

# Type aliases
_float_p = POINTER(c_float)
_size_t = c_size_t
_uint16_p = POINTER(c_uint16)
_int8_p = POINTER(c_int8)

# C signatures
_lib.hsd_euclidean_f32.argtypes = (_float_p, _float_p, _size_t, _float_p)
_lib.hsd_cosine_f32.argtypes = (_float_p, _float_p, _size_t, _float_p)
_lib.hsd_manhattan_f32.argtypes = (_float_p, _float_p, _size_t, _float_p)
_lib.hsd_dot_f32.argtypes = (_float_p, _float_p, _size_t, _float_p)
_lib.hsd_jaccard_u16.argtypes = (_uint16_p, _uint16_p, _size_t, _float_p)
_lib.hsd_hamming_i8.argtypes = (_int8_p, _int8_p, _size_t, _float_p)
_lib.hsd_normalize_l2_f32.argtypes = (_float_p, _size_t)

# Return types
_lib.hsd_euclidean_f32.restype = c_int
_lib.hsd_cosine_f32.restype = c_int
_lib.hsd_manhattan_f32.restype = c_int
_lib.hsd_dot_f32.restype = c_int
_lib.hsd_jaccard_u16.restype = c_int
_lib.hsd_hamming_i8.restype = c_int
_lib.hsd_normalize_l2_f32.restype = c_int

# Helpers
def _check_1d(a, dtype):
    if not isinstance(a, np.ndarray):
        raise TypeError("input must be a NumPy array")
    if a.dtype != dtype:
        raise TypeError(f"array must be {dtype}")
    if a.ndim != 1:
        raise ValueError("array must be 1-dimensional")
    if not a.flags['C_CONTIGUOUS']:
        a = np.ascontiguousarray(a, dtype=dtype)
    return a

def _pair_args(a, b, dtype):
    a = _check_1d(a, dtype)
    b = _check_1d(b, dtype)
    if a.shape != b.shape:
        raise ValueError("arrays must have same shape")
    if a.size == 0:
        raise ValueError("zero-length vectors are not allowed")
    return a, b, a.size

def _check_writeable(vec):
    if not isinstance(vec, np.ndarray):
        raise TypeError("input must be a NumPy array")
    if vec.dtype != np.float32:
        raise TypeError("array must be float32")
    if vec.ndim != 1:
        raise ValueError("array must be 1-dimensional")
    if not vec.flags['C_CONTIGUOUS']:
        vec = np.ascontiguousarray(vec, dtype=np.float32)
    if not vec.flags['WRITEABLE']:
        raise ValueError("array must be writeable")
    return vec, vec.size

# Wrappers
def euclidean_f32(a, b):
    a, b, n = _pair_args(a, b, np.float32)
    out = c_float()
    if _lib.hsd_euclidean_f32(a.ctypes.data_as(_float_p), b.ctypes.data_as(_float_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_euclidean_f32 failed")
    return out.value

def cosine_f32(a, b):
    a, b, n = _pair_args(a, b, np.float32)
    out = c_float()
    if _lib.hsd_cosine_f32(a.ctypes.data_as(_float_p), b.ctypes.data_as(_float_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_cosine_f32 failed")
    return out.value

def manhattan_f32(a, b):
    a, b, n = _pair_args(a, b, np.float32)
    out = c_float()
    if _lib.hsd_manhattan_f32(a.ctypes.data_as(_float_p), b.ctypes.data_as(_float_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_manhattan_f32 failed")
    return out.value

def dot_f32(a, b):
    a, b, n = _pair_args(a, b, np.float32)
    out = c_float()
    if _lib.hsd_dot_f32(a.ctypes.data_as(_float_p), b.ctypes.data_as(_float_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_dot_f32 failed")
    return out.value

def jaccard_u16(a, b):
    a, b, n = _pair_args(a, b, np.uint16)
    out = c_float()
    if _lib.hsd_jaccard_u16(a.ctypes.data_as(_uint16_p), b.ctypes.data_as(_uint16_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_jaccard_u16 failed")
    return out.value

def hamming_i8(a, b):
    a, b, n = _pair_args(a, b, np.int8)
    out = c_float()
    if _lib.hsd_hamming_i8(a.ctypes.data_as(_int8_p), b.ctypes.data_as(_int8_p), n, byref(out)) != 0:
        raise RuntimeError("hsd_hamming_i8 failed")
    return out.value

def normalize_l2_f32(vec):
    vec, n = _check_writeable(vec)
    if n == 0 or np.allclose(vec, 0):
        raise ValueError("cannot normalize zero vector")
    if _lib.hsd_normalize_l2_f32(vec.ctypes.data_as(_float_p), n) != 0:
        raise RuntimeError("hsd_normalize_l2_f32 failed")
