import numpy as np
import pytest


def _calculate_hamming_impl(a_u8: np.ndarray, b_u8: np.ndarray) -> int:
    if a_u8.size != b_u8.size:
        raise ValueError("Arrays must have the same size for Hamming distance")
    if a_u8.ndim != 1:
        raise ValueError("Input arrays must be 1-dimensional")
    count = 0
    for i in range(a_u8.size):
        count += bin(a_u8[i] ^ b_u8[i]).count("1")
    return count


def _calculate_jaccard_similarity_impl(a_u16: np.ndarray, b_u16: np.ndarray) -> float:
    if a_u16.size != b_u16.size:
        raise ValueError("Arrays must have the same size for Jaccard similarity")
    if a_u16.ndim != 1:
        raise ValueError("Input arrays must be 1-dimensional")
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


@pytest.fixture
def calculate_hamming():
    return _calculate_hamming_impl


@pytest.fixture
def calculate_jaccard_similarity():
    return _calculate_jaccard_similarity_impl
