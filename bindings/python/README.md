## HsdPy

[![License: MIT](https://img.shields.io/badge/License-MIT-007ec6.svg)](../../LICENSE)
[![Python version](https://img.shields.io/badge/Python-%3E=3.10-blue)](https://github.com/habedi/hsdlib)
[![PyPI version](https://badge.fury.io/py/hsdpy.svg)](https://badge.fury.io/py/hsdpy)
[![Pip downloads](https://img.shields.io/pypi/dm/hsdpy.svg)](https://pypi.org/project/hsdpy)

HsdPy library allows users to use [Hsdlib](https://github.com/habedi/hsdlib) in Python.

### Installation

```bash
pip install hsdpy
```

### Examples

Below is a simple usage example of HsdPy:

```python
import hsdpy

# HsdPy uses NumPy for array handling
import numpy as np

# Create two NumPy arrays of float32 type
a = np.array([1.0, 2.0, 3.0], dtype=np.float32)
b = np.array([4.0, 5.0, 6.0], dtype=np.float32)

# Calculate the euclidean distance between the two arrays
dist_euc = hsdpy.dist_sqeuclidean_f32(a, b) ** 0.5

# Calculate Manhattan distance
dist_man = hsdpy.dist_manhattan_f32(a, b)

# Let's see the results
print(f"Euclidean distance: {dist_euc}")  # 5.196
print(f"Manhattan distance: {dist_man}")  # 9.0

# See the SIMD backend in use
print(f"Backend: {hsdpy.get_backend()}")
```

Check out [hsdpy_example.py](../../examples/hsdpy_example.py) for more detailed examples.

### API Summary

| Function                     | Description                                          | Input Types (`np.ndarray`) | Return Type |
|------------------------------|------------------------------------------------------|----------------------------|-------------|
| `dist_sqeuclidean_f32(a, b)` | Computes squared Euclidean distance between vectors. | `np.float32`               | `float`     |
| `dist_manhattan_f32(a, b)`   | Computes Manhattan distance between vectors.         | `np.float32`               | `float`     |
| `dist_hamming_u8(a, b)`      | Computes Hamming distance between binary vectors.    | `np.uint8`                 | `int`       |
| `sim_dot_f32(a, b)`          | Computes dot product between vectors.                | `np.float32`               | `float`     |
| `sim_cosine_f32(a, b)`       | Computes cosine similarity between vectors.          | `np.float32`               | `float`     |
| `sim_jaccard_u16(a, b)`      | Computes Jaccard similarity between integer vectors. | `np.uint16`                | `float`     |
| `get_backend()`              | Returns information about the backend in use.        | `None`                     | `str`       |
| `get_library_info()`         | Returns information about the loaded library.        | `None`                     | `dict`      |

#### Notes

- HsdPy provides the `HsdError` exception class for error handling. It is a custom exception class wraps the Hsdlib
  error codes to make them more Pythonic.
- All distance and similarity functions expect one-dimensional NumPy arrays as input.
- Functions will raise `NotImplementedError` if the corresponding Hsdlib function is not implemented for the given data
  type.

### License

HsdPy is licensed under the [MIT License](../../LICENSE).
