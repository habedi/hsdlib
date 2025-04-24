import numpy as np
import time
from hsdpy import sim_cosine_f32


def cosine_similarity_np(a, b, axis=-1, eps=1e-8):
    dot = np.sum(a * b, axis=axis)
    norm_a = np.linalg.norm(a, axis=axis)
    norm_b = np.linalg.norm(b, axis=axis)
    return dot / (norm_a * norm_b + eps)


N = 1_000_000
a, b = (np.random.rand(N).astype(np.float32) for _ in range(2))

for name, fn in [
    ("HSDLib", lambda: sim_cosine_f32(a, b)),
    ("NumPy", lambda: cosine_similarity_np(a, b)),
]:
    t0 = time.time();
    fn();
    print(f"{name:8s} time:", time.time() - t0)
