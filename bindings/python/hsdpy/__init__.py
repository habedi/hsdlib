import ctypes
import numpy as np

# Import the low-level bindings from the sibling file
from . import _ctypes_bindings as ct
from ._ctypes_bindings import get_library_info


# --- Error Handling ---
class HsdError(Exception):
    """Base exception for hsdpy errors."""

    def __init__(self, status_code, message=""):
        self.status_code = status_code
        self.message = message
        super().__init__(f"HSD Error [{status_code}]: {message}")


# Map status codes to error messages
_ERROR_MAP = {
    ct.HSD_ERR_NULL_PTR: "Null pointer error encountered in C library",
    ct.HSD_ERR_UNSUPPORTED: "Unsupported operation/feature requested from C library",
    ct.HSD_ERR_INVALID_INPUT: "Invalid input data provided to C library (e.g., NaN, Inf, incompatible size)",
    ct.HSD_FAILURE: "General failure reported by C library",
}


def _check_status(status: int):
    """Checks the C function's status code and raises HsdError if it's an error."""
    if status != ct.HSD_SUCCESS:
        message = _ERROR_MAP.get(status, f"Unknown error code {status} from C library")
        raise HsdError(status, message)


# --- NumPy Array Validation and Preparation Helpers ---

def _validate_and_prepare_f32(a: np.ndarray, b: np.ndarray) -> tuple:
    """Validates float32 numpy arrays and prepares ctypes pointers."""
    if not isinstance(a, np.ndarray) or not isinstance(b, np.ndarray):
        raise TypeError("Inputs must be NumPy arrays.")
    if a.shape != b.shape:
        raise ValueError(f"Input arrays must have the same shape ({a.shape} != {b.shape}).")
    if a.ndim != 1:
        raise ValueError("Input arrays must be 1-dimensional.")
    # Ensure correct dtype, allowing safe casting if necessary
    if a.dtype != np.float32:
        try:
            a = a.astype(np.float32, casting='same_kind', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'a' cannot be safely cast to float32 from {a.dtype}")
    if b.dtype != np.float32:
        try:
            b = b.astype(np.float32, casting='same_kind', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'b' cannot be safely cast to float32 from {b.dtype}")

    # Ensure arrays are C-contiguous for direct pointer access
    if not a.flags['C_CONTIGUOUS']:
        a = np.ascontiguousarray(a, dtype=np.float32)
    if not b.flags['C_CONTIGUOUS']:
        b = np.ascontiguousarray(b, dtype=np.float32)

    n = a.size  # Get size after potential modifications
    # Pass null pointers to C if size is 0, otherwise get data pointers
    a_ptr = a.ctypes.data_as(ct.c_float_p) if n > 0 else ctypes.cast(None, ct.c_float_p)
    b_ptr = b.ctypes.data_as(ct.c_float_p) if n > 0 else ctypes.cast(None, ct.c_float_p)

    return a_ptr, b_ptr, n


def _validate_and_prepare_u8(a: np.ndarray, b: np.ndarray) -> tuple:
    """Validates uint8 numpy arrays and prepares ctypes pointers."""
    if not isinstance(a, np.ndarray) or not isinstance(b, np.ndarray):
        raise TypeError("Inputs must be NumPy arrays.")
    if a.shape != b.shape:
        raise ValueError(f"Input arrays must have the same shape ({a.shape} != {b.shape}).")
    if a.ndim != 1:
        raise ValueError("Input arrays must be 1-dimensional.")
    if a.dtype != np.uint8:
        try:
            a = a.astype(np.uint8, casting='safe', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'a' cannot be safely cast to uint8 from {a.dtype}")
    if b.dtype != np.uint8:
        try:
            b = b.astype(np.uint8, casting='safe', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'b' cannot be safely cast to uint8 from {b.dtype}")

    if not a.flags['C_CONTIGUOUS']:
        a = np.ascontiguousarray(a, dtype=np.uint8)
    if not b.flags['C_CONTIGUOUS']:
        b = np.ascontiguousarray(b, dtype=np.uint8)

    n = a.size
    a_ptr = a.ctypes.data_as(ct.c_uint8_p) if n > 0 else ctypes.cast(None, ct.c_uint8_p)
    b_ptr = b.ctypes.data_as(ct.c_uint8_p) if n > 0 else ctypes.cast(None, ct.c_uint8_p)

    return a_ptr, b_ptr, n


def _validate_and_prepare_u16(a: np.ndarray, b: np.ndarray) -> tuple:
    """Validates uint16 numpy arrays and prepares ctypes pointers."""
    if not isinstance(a, np.ndarray) or not isinstance(b, np.ndarray):
        raise TypeError("Inputs must be NumPy arrays.")
    if a.shape != b.shape:
        raise ValueError(f"Input arrays must have the same shape ({a.shape} != {b.shape}).")
    if a.ndim != 1:
        raise ValueError("Input arrays must be 1-dimensional.")
    if a.dtype != np.uint16:
        try:
            a = a.astype(np.uint16, casting='safe', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'a' cannot be safely cast to uint16 from {a.dtype}")
    if b.dtype != np.uint16:
        try:
            b = b.astype(np.uint16, casting='safe', copy=False)
        except TypeError:
            raise TypeError(f"Input array 'b' cannot be safely cast to uint16 from {b.dtype}")

    if not a.flags['C_CONTIGUOUS']:
        a = np.ascontiguousarray(a, dtype=np.uint16)
    if not b.flags['C_CONTIGUOUS']:
        b = np.ascontiguousarray(b, dtype=np.uint16)

    n = a.size
    a_ptr = a.ctypes.data_as(ct.c_uint16_p) if n > 0 else ctypes.cast(None, ct.c_uint16_p)
    b_ptr = b.ctypes.data_as(ct.c_uint16_p) if n > 0 else ctypes.cast(None, ct.c_uint16_p)

    return a_ptr, b_ptr, n


# --- Public API Functions ---

def dist_sqeuclidean_f32(a: np.ndarray, b: np.ndarray) -> float:
    """
    Calculates the squared Euclidean distance between two float32 vectors.

    Args:
        a: NumPy array (1D, float32).
        b: NumPy array (1D, float32).

    Returns:
        The squared Euclidean distance.

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to float32.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_dist_sqeuclidean_f32 is None:
        raise NotImplementedError("hsd_dist_sqeuclidean_f32 not available in C library")
    # Validate inputs and get ctypes pointers/size
    a_ptr, b_ptr, n = _validate_and_prepare_f32(a, b)
    # Prepare a ctypes variable to receive the result
    result = ctypes.c_float()
    # Call the C function
    status = ct.hsd_dist_sqeuclidean_f32(a_ptr, b_ptr, n, ctypes.byref(result))
    # Check the return status
    _check_status(status)
    # Return the value from the result variable
    return result.value


def dist_manhattan_f32(a: np.ndarray, b: np.ndarray) -> float:
    """
    Calculates the Manhattan (L1) distance between two float32 vectors.

    Args:
        a: NumPy array (1D, float32).
        b: NumPy array (1D, float32).

    Returns:
        The Manhattan distance.

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to float32.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_dist_manhattan_f32 is None:
        raise NotImplementedError("hsd_dist_manhattan_f32 not available in C library")
    a_ptr, b_ptr, n = _validate_and_prepare_f32(a, b)
    result = ctypes.c_float()
    status = ct.hsd_dist_manhattan_f32(a_ptr, b_ptr, n, ctypes.byref(result))
    _check_status(status)
    return result.value


def dist_hamming_u8(a: np.ndarray, b: np.ndarray) -> int:
    """
    Calculates the Hamming distance between two uint8 vectors.

    Args:
        a: NumPy array (1D, uint8).
        b: NumPy array (1D, uint8).

    Returns:
        The Hamming distance (int).

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to uint8.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_dist_hamming_u8 is None:
        raise NotImplementedError("hsd_dist_hamming_u8 not available in C library")
    a_ptr, b_ptr, n = _validate_and_prepare_u8(a, b)
    result = ctypes.c_uint64()  # C function returns uint64_t
    status = ct.hsd_dist_hamming_u8(a_ptr, b_ptr, n, ctypes.byref(result))
    _check_status(status)
    return result.value  # Returns as Python int


def sim_dot_f32(a: np.ndarray, b: np.ndarray) -> float:
    """
    Calculates the dot product similarity between two float32 vectors.

    Args:
        a: NumPy array (1D, float32).
        b: NumPy array (1D, float32).

    Returns:
        The dot product.

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to float32.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_sim_dot_f32 is None:
        raise NotImplementedError("hsd_sim_dot_f32 not available in C library")
    a_ptr, b_ptr, n = _validate_and_prepare_f32(a, b)
    result = ctypes.c_float()
    status = ct.hsd_sim_dot_f32(a_ptr, b_ptr, n, ctypes.byref(result))
    _check_status(status)
    return result.value


def sim_cosine_f32(a: np.ndarray, b: np.ndarray) -> float:
    """
    Calculates the cosine similarity between two float32 vectors.

    Args:
        a: NumPy array (1D, float32).
        b: NumPy array (1D, float32).

    Returns:
        The cosine similarity [-1.0, 1.0].

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to float32.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_sim_cosine_f32 is None:
        raise NotImplementedError("hsd_sim_cosine_f32 not available in C library")
    a_ptr, b_ptr, n = _validate_and_prepare_f32(a, b)
    result = ctypes.c_float()
    status = ct.hsd_sim_cosine_f32(a_ptr, b_ptr, n, ctypes.byref(result))
    _check_status(status)
    return result.value


def sim_jaccard_u16(a: np.ndarray, b: np.ndarray) -> float:
    """
    Calculates the Jaccard similarity (Tanimoto coefficient) between
    two uint16 vectors.

    Args:
        a: NumPy array (1D, uint16).
        b: NumPy array (1D, uint16).

    Returns:
        The Jaccard similarity [0.0, 1.0].

    Raises:
        TypeError: If inputs are not NumPy arrays or cannot be cast to uint16.
        ValueError: If arrays have different shapes or are not 1D.
        HsdError: If the underlying C library call fails.
    """
    if ct.hsd_sim_jaccard_u16 is None:
        raise NotImplementedError("hsd_sim_jaccard_u16 not available in C library")
    a_ptr, b_ptr, n = _validate_and_prepare_u16(a, b)
    result = ctypes.c_float()
    status = ct.hsd_sim_jaccard_u16(a_ptr, b_ptr, n, ctypes.byref(result))
    _check_status(status)
    return result.value


def get_backend() -> str:
    """Returns the best available CPU backend detected by the C library."""
    if ct.hsd_get_backend is None:
        # Provide a default or raise an error if the function must exist
        # raise NotImplementedError("hsd_get_backend not available in C library")
        return "unknown (hsd_get_backend function not found in C library)"
    backend_bytes = ct.hsd_get_backend()
    # Decode from bytes (assuming UTF-8) to a Python string
    return backend_bytes.decode('utf-8') if backend_bytes else "unknown"


# --- Expose Public Interface ---
# These are the functions users will import from hsdpy
__all__ = [
    "dist_sqeuclidean_f32",
    "dist_manhattan_f32",
    "dist_hamming_u8",
    "sim_dot_f32",
    "sim_cosine_f32",
    "sim_jaccard_u16",
    "get_backend",
    "HsdError",
    "get_library_info"
]

# Add version information
__version__ = "0.1.0"  # Example version
