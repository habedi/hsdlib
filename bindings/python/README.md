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

### Simple Example

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

### License

HsdPy is licensed under the [MIT License](../../LICENSE).
