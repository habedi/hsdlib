"""
hsdpy: Python ctypes bindings for the Hsdlib C library.
"""

from ._ctypes_bindings import (
    euclidean_f32,
    cosine_f32,
    manhattan_f32,
    dot_f32,
    jaccard_u16,
    hamming_i8,
    normalize_l2_f32,
)

__all__ = [
    "euclidean_f32",
    "cosine_f32",
    "manhattan_f32",
    "dot_f32",
    "jaccard_u16",
    "hamming_i8",
    "normalize_l2_f32",
]

__version__ = "0.1.0"
