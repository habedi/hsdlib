import pytest
import numpy as np
import hsdpy
from scipy.spatial.distance import euclidean, cityblock, cosine

# ----------------------
# Euclidean Distance
# ----------------------

def test_euclidean_f32_basic():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    expected = euclidean(a, b)
    assert hsdpy.euclidean_f32(a, b) == pytest.approx(expected)

def test_euclidean_f32_reverse():
    a = np.array([0, 1, 2, 3], dtype=np.float32)
    b = a[::-1]
    expected = euclidean(a, b)
    assert hsdpy.euclidean_f32(a, b) == pytest.approx(expected)

def test_euclidean_f32_shape_mismatch():
    a = np.array([1, 2], dtype=np.float32)
    b = np.array([1, 2, 3], dtype=np.float32)
    with pytest.raises(ValueError):
        hsdpy.euclidean_f32(a, b)

def test_euclidean_f32_dtype_error():
    a = np.array([1, 2, 3], dtype=np.float64)
    b = np.array([1, 2, 3], dtype=np.float64)
    with pytest.raises(TypeError):
        hsdpy.euclidean_f32(a, b)

def test_euclidean_f32_zero_length():
    a = np.array([], dtype=np.float32)
    b = np.array([], dtype=np.float32)
    with pytest.raises(ValueError):
        hsdpy.euclidean_f32(a, b)

# ----------------------
# Cosine Distance
# ----------------------

def test_cosine_f32_identical():
    a = np.array([1, 0], dtype=np.float32)
    b = np.array([1, 0], dtype=np.float32)
    expected = cosine(a, b)
    assert hsdpy.cosine_f32(a, b) == pytest.approx(expected, abs=1e-6)

def test_cosine_f32_opposite():
    a = np.array([1, 0], dtype=np.float32)
    b = np.array([-1, 0], dtype=np.float32)
    expected = cosine(a, b)
    assert hsdpy.cosine_f32(a, b) == pytest.approx(expected, abs=1e-6)

# ----------------------
# Dot Product
# ----------------------

def test_dot_f32_positive():
    a = np.array([1, 2], dtype=np.float32)
    b = np.array([3, 4], dtype=np.float32)
    expected = np.dot(a, b)
    assert hsdpy.dot_f32(a, b) == pytest.approx(expected)

def test_dot_f32_negative():
    a = np.array([-1, -2], dtype=np.float32)
    b = np.array([3, 4], dtype=np.float32)
    expected = np.dot(a, b)
    assert hsdpy.dot_f32(a, b) == pytest.approx(expected)

# ----------------------
# Manhattan Distance
# ----------------------

def test_manhattan_f32_basic():
    a = np.array([1, 2, 3], dtype=np.float32)
    b = np.array([4, 0, 3], dtype=np.float32)
    expected = cityblock(a, b)
    assert hsdpy.manhattan_f32(a, b) == pytest.approx(expected)

# ----------------------
# Hamming (int8)
# ----------------------

def test_hamming_i8_basic():
    a = np.array([-86], dtype=np.int8)  # 0b10101010
    b = np.array([-16], dtype=np.int8)  # 0b11110000
    a_u8 = a.astype(np.uint8)
    b_u8 = b.astype(np.uint8)
    expected = bin(a_u8[0] ^ b_u8[0]).count("1")
    assert hsdpy.hamming_i8(a, b) == pytest.approx(expected)

def test_hamming_i8_extreme_values():
    a = np.array([-128], dtype=np.int8)  # 0b10000000
    b = np.array([127], dtype=np.int8)   # 0b01111111
    a_u8 = a.astype(np.uint8)
    b_u8 = b.astype(np.uint8)
    expected = bin(a_u8[0] ^ b_u8[0]).count("1")
    assert hsdpy.hamming_i8(a, b) == pytest.approx(expected)

# ----------------------
# Jaccard (uint16)
# ----------------------

def test_jaccard_u16_basic():
    a = np.array([0b1111], dtype=np.uint16)
    b = np.array([0b1100], dtype=np.uint16)
    a_u16 = a.astype(np.uint16)
    b_u16 = b.astype(np.uint16)
    intersection = bin(a_u16[0] & b_u16[0]).count("1")
    union = bin(a_u16[0] | b_u16[0]).count("1")
    expected = 1.0 - (intersection / union)
    assert hsdpy.jaccard_u16(a, b) == pytest.approx(expected, rel=1e-6)

# ----------------------
# Normalize (in-place)
# ----------------------

def test_normalize_l2_f32_unit_vector():
    vec = np.array([3, 4], dtype=np.float32)
    hsdpy.normalize_l2_f32(vec)
    assert np.linalg.norm(vec) == pytest.approx(1.0)

def test_normalize_l2_f32_zeros():
    vec = np.zeros(4, dtype=np.float32)
    with pytest.raises(ValueError):
        hsdpy.normalize_l2_f32(vec)
