# python/tests/test_hsdpy.py
import numpy as np
import pytest
from scipy.spatial.distance import cityblock as scipy_manhattan
from scipy.spatial.distance import cosine as scipy_cosine_dist
from scipy.spatial.distance import euclidean as scipy_euclidean

# Import the package, functions are accessed via hsdpy.<func_name>
import hsdpy


# Helper for manual hamming distance
def calculate_hamming(a_u8: np.ndarray, b_u8: np.ndarray) -> int:
    if a_u8.size != b_u8.size:
        raise ValueError("Arrays must have the same size")
    count = 0
    for i in range(a_u8.size):
        count += bin(a_u8[i] ^ b_u8[i]).count("1")
    return count


# Helper for manual Jaccard similarity (Tanimoto coefficient)
def calculate_jaccard_similarity(a_u16: np.ndarray, b_u16: np.ndarray) -> float:
    if a_u16.size != b_u16.size:
        raise ValueError("Arrays must have the same size")
    if a_u16.size == 0:
        return 1.0

    a_f64 = a_u16.astype(np.float64)
    b_f64 = b_u16.astype(np.float64)

    dot = np.dot(a_f64, b_f64)
    norm_a_sq = np.dot(a_f64, a_f64)
    norm_b_sq = np.dot(b_f64, b_f64)

    denominator = norm_a_sq + norm_b_sq - dot

    if norm_a_sq == 0 and norm_b_sq == 0:
        return 1.0

    if denominator < 1e-9:
        similarity = 1.0
    else:
        similarity = dot / denominator

    return min(max(0.0, similarity), 1.0)


# -----------------------------------
# Squared Euclidean Distance (f32)
# -----------------------------------

def test_sqeuclidean_f32_basic():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    expected_euclidean = scipy_euclidean(a, b)
    expected_sq = expected_euclidean ** 2
    # Use the exported name: dist_sqeuclidean_f32
    assert hsdpy.dist_sqeuclidean_f32(a, b) == pytest.approx(expected_sq)


def test_sqeuclidean_f32_different():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([4, 5, 6], dtype=np.float32)
    expected_sq = np.sum((a - b) ** 2)
    # Use the exported name: dist_sqeuclidean_f32
    assert hsdpy.dist_sqeuclidean_f32(a, b) == pytest.approx(expected_sq)


def test_sqeuclidean_f32_reverse():
    a = np.array([0, 1, 2, 3], dtype=np.float32)
    b = a[::-1].copy()
    expected_sq = np.sum((a - b) ** 2)
    # Use the exported name: dist_sqeuclidean_f32
    assert hsdpy.dist_sqeuclidean_f32(a, b) == pytest.approx(expected_sq)


def test_sqeuclidean_f32_shape_mismatch():
    a = np.array([1, 2], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    with pytest.raises(ValueError, match="same shape"):
        # Use the exported name: dist_sqeuclidean_f32
        hsdpy.dist_sqeuclidean_f32(a, b)


def test_sqeuclidean_f32_dtype_casting_ok():
    a = np.array([1, 2, 3], dtype=np.float64)
    b = np.array([4, 5, 6], dtype=np.float64)
    expected_sq = np.sum((a.astype(np.float32) - b.astype(np.float32)) ** 2)
    # Use the exported name: dist_sqeuclidean_f32
    assert hsdpy.dist_sqeuclidean_f32(a, b) == pytest.approx(expected_sq)


def test_sqeuclidean_f32_dtype_error_unsafe():
    a = np.array([1 + 1j, 2], dtype=np.complex64)
    b = np.array([1, 2], dtype=np.complex64)
    with pytest.raises(TypeError, match="cannot be safely cast"):
        # Use the exported name: dist_sqeuclidean_f32
        hsdpy.dist_sqeuclidean_f32(a, b)


def test_sqeuclidean_f32_zero_length():
    a = np.array([], dtype=np.float32)
    b = np.array([], dtype=np.float32)
    expected = 0.0
    # Use the exported name: dist_sqeuclidean_f32
    assert hsdpy.dist_sqeuclidean_f32(a, b) == pytest.approx(expected)


# -----------------------------------
# Cosine Similarity (f32)
# -----------------------------------

def test_cosine_f32_identical():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    expected_similarity = 1.0
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


def test_cosine_f32_opposite():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([-1, -2, -3], dtype=np.float32)
    expected_similarity = -1.0
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


def test_cosine_f32_orthogonal():
    a = np.array([1, 0], dtype=np.float32)
    b = np.array([0, 1], dtype=np.float32)
    expected_similarity = 0.0
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


def test_cosine_f32_general():
    a = np.array([1, 2, 3, 4], dtype=np.float32)
    b = np.array([5, 6, 7, 8], dtype=np.float32)
    expected_dist = scipy_cosine_dist(a, b)
    expected_similarity = 1.0 - expected_dist
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


def test_cosine_f32_zero_vector_a():
    a = np.array([0, 0, 0], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    expected_similarity = 0.0
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


def test_cosine_f32_zero_vector_both():
    a = np.array([0, 0], dtype=np.float32)
    b = np.array([0, 0], dtype=np.float32)
    expected_similarity = 1.0
    # Use the exported name: sim_cosine_f32
    assert hsdpy.sim_cosine_f32(a, b) == pytest.approx(expected_similarity, abs=1e-6)


# ----------------------
# Dot Product (f32)
# ----------------------

def test_dot_f32_positive():
    a = np.array([1, 2, 5, 2], dtype=np.float32)
    b = np.array([3, 4, 1, 6], dtype=np.float32)
    expected = np.dot(a, b)
    # Use the exported name: sim_dot_f32
    assert hsdpy.sim_dot_f32(a, b) == pytest.approx(expected)


def test_dot_f32_negative():
    a = np.array([-1, -2, 1], dtype=np.float32)
    b = np.array([3, 4, -2], dtype=np.float32)
    expected = np.dot(a, b)
    # Use the exported name: sim_dot_f32
    assert hsdpy.sim_dot_f32(a, b) == pytest.approx(expected)


def test_dot_f32_zero_length():
    a = np.array([], dtype=np.float32)
    b = np.array([], dtype=np.float32)
    expected = 0.0
    # Use the exported name: sim_dot_f32
    assert hsdpy.sim_dot_f32(a, b) == pytest.approx(expected)


# ----------------------
# Manhattan Distance (f32)
# ----------------------

def test_manhattan_f32_basic():
    a = np.array([1, -2, 3, 0], dtype=np.float32)
    b = np.array([4, 0, -3, 5], dtype=np.float32)
    expected = scipy_manhattan(a, b)
    # Use the exported name: dist_manhattan_f32
    assert hsdpy.dist_manhattan_f32(a, b) == pytest.approx(expected)


def test_manhattan_f32_identical():
    a = np.array([10, 20, 30], dtype=np.float32)
    b = np.array([10, 20, 30], dtype=np.float32)
    expected = scipy_manhattan(a, b)
    # Use the exported name: dist_manhattan_f32
    assert hsdpy.dist_manhattan_f32(a, b) == pytest.approx(expected)


def test_manhattan_f32_zero_length():
    a = np.array([], dtype=np.float32)
    b = np.array([], dtype=np.float32)
    expected = 0.0
    # Use the exported name: dist_manhattan_f32
    assert hsdpy.dist_manhattan_f32(a, b) == pytest.approx(expected)


# ----------------------
# Hamming (uint8)
# ----------------------

def test_hamming_u8_basic():
    a_i8 = np.array([-86, 0, 127], dtype=np.int8)
    b_i8 = np.array([-16, 0, -128], dtype=np.int8)
    a = a_i8.astype(np.uint8)
    b = b_i8.astype(np.uint8)
    expected = calculate_hamming(a, b)
    # Use the exported name: dist_hamming_u8
    assert hsdpy.dist_hamming_u8(a, b) == expected


def test_hamming_u8_identical():
    a = np.array([1, 2, 3, 255, 0], dtype=np.uint8)
    b = np.array([1, 2, 3, 255, 0], dtype=np.uint8)
    expected = calculate_hamming(a, b)
    # Use the exported name: dist_hamming_u8
    assert hsdpy.dist_hamming_u8(a, b) == expected


def test_hamming_u8_zero_length():
    a = np.array([], dtype=np.uint8)
    b = np.array([], dtype=np.uint8)
    expected = 0
    # Use the exported name: dist_hamming_u8
    assert hsdpy.dist_hamming_u8(a, b) == expected


def test_hamming_u8_dtype_error():
    a = np.array([1, 2], dtype=np.int8)
    b = np.array([3, 4], dtype=np.int8)
    with pytest.raises(TypeError, match="cannot be safely cast to uint8"):
        # Use the exported name: dist_hamming_u8
        hsdpy.dist_hamming_u8(a, b)


# ----------------------
# Jaccard (uint16)
# ----------------------

def test_jaccard_u16_basic():
    a = np.array([10, 20, 0, 30, 5], dtype=np.uint16)
    b = np.array([10, 25, 5, 30, 0], dtype=np.uint16)
    expected = calculate_jaccard_similarity(a, b)
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected, abs=1e-6)


def test_jaccard_u16_identical():
    a = np.array([5, 0, 100], dtype=np.uint16)
    b = np.array([5, 0, 100], dtype=np.uint16)
    expected = calculate_jaccard_similarity(a, b)
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected, abs=1e-6)


def test_jaccard_u16_disjoint_non_zero():
    a = np.array([10, 0, 30], dtype=np.uint16)
    b = np.array([0, 20, 0], dtype=np.uint16)
    expected = calculate_jaccard_similarity(a, b)
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected, abs=1e-6)


def test_jaccard_u16_zero_vectors():
    a = np.array([0, 0], dtype=np.uint16)
    b = np.array([0, 0], dtype=np.uint16)
    expected = calculate_jaccard_similarity(a, b)
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected, abs=1e-6)


def test_jaccard_u16_one_zero_vector():
    a = np.array([10, 20], dtype=np.uint16)
    b = np.array([0, 0], dtype=np.uint16)
    expected = calculate_jaccard_similarity(a, b)
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected, abs=1e-6)


def test_jaccard_u16_zero_length():
    a = np.array([], dtype=np.uint16)
    b = np.array([], dtype=np.uint16)
    expected = 1.0
    # Use the exported name: sim_jaccard_u16
    assert hsdpy.sim_jaccard_u16(a, b) == pytest.approx(expected)
