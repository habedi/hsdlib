<div align="center">
  <picture>
    <img alt="Hsdlib Logo" src="logo.svg" height="50%" width="50%">
  </picture>
<br>

<h2>Hsdlib</h2>

[![Tests](https://img.shields.io/github/actions/workflow/status/habedi/hsdlib/tests_amd64.yml?label=tests&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/hsdlib/actions/workflows/tests_amd64.yml)
[![Benches](https://img.shields.io/github/actions/workflow/status/habedi/hsdlib/benches_amd64.yml?label=benches&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/hsdlib/actions/workflows/benches_amd64.yml)
[![Lints](https://img.shields.io/github/actions/workflow/status/habedi/hsdlib/lints.yml?label=lints&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/hsdlib/actions/workflows/lints.yml)
[![Code Coverage](https://img.shields.io/codecov/c/github/habedi/hsdlib?label=coverage&style=flat&labelColor=282c34&logo=codecov)](https://codecov.io/gh/habedi/hsdlib)
[![CodeFactor](https://img.shields.io/codefactor/grade/github/habedi/hsdlib?label=code%20quality&style=flat&labelColor=282c34&logo=codefactor)](https://www.codefactor.io/repository/github/habedi/hsdlib)
[![Docs](https://img.shields.io/badge/docs-latest-007ec6?label=docs&style=flat&labelColor=282c34&logo=readthedocs)](docs)
[![License](https://img.shields.io/badge/license-MIT-007ec6?label=license&style=flat&labelColor=282c34&logo=open-source-initiative)](https://github.com/habedi/hsdlib)
[![Release](https://img.shields.io/github/release/habedi/hsdlib.svg?label=release&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/hsdlib/releases/latest)

Hardware-accelerated distance metrics and similarity measures in C

</div>

---

Hsdlib is a C library that provides hardware-accelerated implementations of popular distance metrics and
similarity measures for high-dimensional data.
It automatically selects and uses the best implementation based on the available CPU features for maximizing
performance.

### Features

- Support for popular distances and similarity measures including Euclidean, Manhattan, and cosine distances, and dot
  product and Jaccard similarity measures
- Support for AVX, AVX2, AVX512, NEON, and SVE instructions
- Support for dynamic dispatching to select the best implementation based on the available CPU features

---

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to make a contribution.

### License

Hsdlib is licensed under the MIT License ([LICENSE](LICENSE)).
