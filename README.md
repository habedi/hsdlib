## C Project Template

<div align="center">
  <picture>
    <img alt="C Logo" src="logo.svg" height="25%" width="25%">
  </picture>
</div>
<br>

[![Tests](https://img.shields.io/github/actions/workflow/status/habedi/template-c-project/tests.yml?label=tests&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/template-c-project/actions/workflows/tests.yml)
[![Lints](https://img.shields.io/github/actions/workflow/status/habedi/template-c-project/lints.yml?label=lints&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/template-c-project/actions/workflows/lints.yml)
[![Code Coverage](https://img.shields.io/codecov/c/github/habedi/template-c-project?label=coverage&style=flat&labelColor=282c34&logo=codecov)](https://codecov.io/gh/habedi/template-c-project)
[![CodeFactor](https://img.shields.io/codefactor/grade/github/habedi/template-c-project?label=code%20quality&style=flat&labelColor=282c34&logo=codefactor)](https://www.codefactor.io/repository/github/habedi/template-c-project)
[![Docs](https://img.shields.io/badge/docs-latest-007ec6?label=docs&style=flat&labelColor=282c34&logo=readthedocs)](docs)
[![License](https://img.shields.io/badge/license-MIT-007ec6?label=license&style=flat&labelColor=282c34&logo=open-source-initiative)](https://github.com/habedi/template-c-project)
[![Release](https://img.shields.io/github/release/habedi/template-c-project.svg?label=release&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/template-c-project/releases/latest)

This is a project template for C projects.
It provides a minimalistic project structure with pre-configured GitHub Actions, Makefile,
and configuration files for a few popular development tools.
I share it here in case it might be useful to others.

### Features

- Minimalistic project structure
- Pre-configured GitHub Actions for linting and testing
- Makefile for managing the development workflow and tasks like code formatting, testing, linting, etc.
- Example configuration files for popular tools like `clang-format`, `clang-tidy`, `Doxygen`, and `valgrind`.
- GitHub badges for tests, code quality and coverage, documentation, etc.
- [Code of Conduct](CODE_OF_CONDUCT.md) and [Contributing Guidelines](CONTRIBUTING.md)

### Getting Started

Check out the [Makefile](Makefile) for available commands to manage the development workflow of the project.

```shell
# Install system and development dependencies (for Debian-based systems)
sudo apt-get install make
make install-deps
```

```shell
# See all available commands and their descriptions
make help
```

### Platform Compatibility

This template should work on most Unix-like environments (like GNU/Linux distributions, BSDs, and macOS),
albeit with some minor modifications.
Windows users might need a Unix-like environment (such as WSL, MSYS2, or Cygwin) to use this template.

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to make a contribution.

### License

This project is licensed under the MIT License ([LICENSE](LICENSE) or https://opensource.org/licenses/MIT)
