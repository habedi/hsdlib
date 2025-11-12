import math
import time

import numpy as np
from hsdpy import sim_cosine_f32
from hsdpy import sim_dot_f32


def cosine_similarity_np(a, b, axis=-1, eps=1e-8):
    dot = np.sum(a * b, axis=axis)
    norm_a = np.linalg.norm(a, axis=axis)
    norm_b = np.linalg.norm(b, axis=axis)
    return dot / (norm_a * norm_b + eps)


# Your implementation of NumPy cosine similarity
def cosine_similarity(a, b):
    dot = np.dot(a, b)
    norm_a = math.sqrt(np.dot(a, a))
    norm_b = math.sqrt(np.dot(b, b))

    return dot / (norm_a * norm_b)


# Cosine similarity using HSDLib dot product
def cosine_similarity_hsdlib(a, b):
    dot = sim_dot_f32(a, b)
    norm_a = math.sqrt(sim_dot_f32(a, a))
    norm_b = math.sqrt(sim_dot_f32(b, b))
    return dot / (norm_a * norm_b)


N = 1_000_000
a, b = (np.random.rand(N).astype(np.float32) for _ in range(2))

for name, fn in [
    ("Hsdlib", lambda: sim_cosine_f32(a, b)),
    ("Hsdlib v2", lambda: cosine_similarity_hsdlib(a, b)),
    ("NumPy", lambda: cosine_similarity_np(a, b)),
    ("NumPy v2", lambda: cosine_similarity(a, b)),
]:
    t0 = time.time();
    fn();
    print(f"{name:8s} time:", time.time() - t0)
