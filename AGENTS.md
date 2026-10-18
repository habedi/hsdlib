# AGENTS.md

This file provides guidance to coding agents collaborating on this repository.

## Mission

Hsdlib is a C library for distance metrics and similarity measures, with Python bindings named HsdPy.
It provides squared Euclidean, Manhattan, and Hamming distances, and dot product, cosine, and Jaccard
similarities. Implementations use scalar code or SIMD backends on AMD64 and AArch64 CPUs, selected at
runtime with an optional manual override.

Priorities, in order:

1. Correct numerical results and consistent behavior across scalar and SIMD backends.
2. Memory safety, including vector tails, unaligned inputs, and output pointers.
3. A stable C API and compatible Python bindings.
4. Performance supported by benchmarks and maintainable code.

## Core Rules

- Use English for code, comments, docs, and tests.
- Prefer small, focused changes over large refactoring.
- Add comments only when they clarify non-obvious behavior.
- Do not add features, error handling, or abstractions beyond what the task needs.
- Keep the C library free of external library dependencies beyond the platform C and math libraries
  and compiler intrinsics. Python runtime and development dependencies belong in `pyproject.toml`.

## Writing Style

- Write in plain English with short sentences and familiar words. Use precise technical terms when they help explain the
  code.
- Use Oxford commas in inline listsl, like "scalar, AVX2, and NEON".
- Do not use em dashes. Use separate sentences, a colon, or a semicolon instead.
- Avoid promotional adjectives and adverbs. Write "SIMD implementation" rather than "blazing-fast SIMD implementation".
  Support performance claims with benchmark results.
- Prefer noun phrases for checklist items, such as "SIMD tail handling". Use direct verbs for instructions, such as "Run
  `make test`."
- Use title case for Markdown headings, such as "Build from Source". Keep articles (a, an, the), coordinating
  conjunctions (and, but, or, nor, so, yet, for), and short prepositions (in, on, at, to, by, of, up, as, from, with,
  into, over) lowercase unless they are the first word.
- Use sentence case for list lead-ins, such as "Backend selection: ...". Do not bold the lead-in.
- Capitalize only the first part of a hyphenated compound: "Floating-point Validation" in a heading, "Floating-point" at
  the start of a sentence, and "floating-point validation" elsewhere.
- Start sentences with a capital letter. Preserve names such as Hsdlib, HsdPy, NumPy, and AArch64, and leave common
  nouns lowercase. Preserve the exact case of code identifiers and commands.
- Write complete sentences in prose. Short labels are appropriate for tables and checklists.
- Use verbs to explain behavior: "Selects a backend based on CPU features." Reserve label-and-colon phrasing for list
  lead-ins, tables, and enumerations.
- Use abbreviations sparingly. Define unfamiliar ones on first use, and prefer direct clauses over long participial
  phrases.

## Repository Layout

- `include/hsdlib.h`: C API declarations, status codes, backend enums, and inline SIMD helpers.
- `src/distance/`: Squared Euclidean, Manhattan, and Hamming implementations.
- `src/similarity/`: Dot product, cosine, and Jaccard implementations.
- `src/utils.c`: CPU feature detection, backend selection, floating-point mode, and logging utilities.
- `tests/`: C tests, with the runner in `main.c` and shared helpers in `test_common.c` and
  `test_common.h`.
- `bindings/python/hsdpy/`: Python API and `ctypes` bindings, including native library loading.
- `bindings/python/tests/`: Python test suite used by `make python-test`.
- `benches/c/` and `benches/py/`: C backend benchmarks and Python comparisons with NumPy.
- `examples/`: C and Python usage examples.
- `.github/workflows/`: AMD64 and AArch64 tests and benchmarks, Python tests, linting, and publishing.
- `Makefile`: Library builds, tests, benchmarks, Python packaging, formatting, and documentation.
- `build.zig` and `build.zig.zon`: Optional Zig 0.17.0 build for C libraries, tests, and the C example.
  Default installation goes under `zig-out/`. Zig packages omit `tests/` and `examples/`; their build
  targets are registered only when the corresponding sources are present.
- `pyproject.toml`: Python packaging, dependencies, Ruff, and Pyright configuration.
- `flake.nix`: Nix development shell and Nix formatter. Enter with `nix develop`.
- `.clang-format`: Google base style, four-space indentation, and a 100-column limit.
- `Doxyfile`: Doxygen configuration, with generated HTML under `docs/html/`.

## Architecture and Compatibility

### Compiled Library

Consumers include `hsdlib.h` and link against the static or shared library produced under `lib/`.
Build artifacts and the C test runner go under `obj/` and `bin/`, respectively.

The C library uses C11, compiler intrinsics, and architecture-specific feature detection. The Makefile
prefers GCC, falls back to Clang, and allows an override such as `make test CC=clang`. Tests and the C
example use `-std=gnu2x`. Common flags include `-Wall -Wextra -pedantic`. Keep ordinary code at the
compiler's baseline CPU target; SIMD implementations enable their own features through target
attributes and runtime checks. Global `-march=native` or `-mfma` flags can break distributed binaries.

### Public API and Python Bindings

`HSD_VERSION` in `include/hsdlib.h` is the source of truth for the release version. `setup.py` reads it
for Python packaging, and `MANIFEST.in` includes the header in source distributions. Keep the literal
version in `build.zig.zon` and the release tag in sync. Run `uv run --extra dev python tests/test_version.py`
when changing version metadata or packaging.

Preserve public function signatures, status codes, enum values, and result types in `include/hsdlib.h`.
Metric functions return `hsd_status_t` and write results through output pointers. Changes to the C API
must also reach the `ctypes` signatures and Python wrappers in
`bindings/python/hsdpy/_ctypes_bindings.py`, the exports in `__init__.py`, relevant tests, and examples.

HsdPy depends on NumPy and loads a compiled shared library. `make python-build` rebuilds the C library,
copies the shared library into the Python package, and builds a wheel. `HSDLIB_PATH` can point to a
specific shared library when testing local changes.

### SIMD Dispatch and Numerical Behavior

Metric implementations contain scalar and architecture-specific paths. Backend resolution is cached
through atomic function pointers. Check both the resolver and the implementation when changing a
metric, and set manual backend overrides before the first metric call in a process.

Preserve input validation, zero-length behavior, floating-point tolerances, and scalar fallback when
a requested backend is unavailable. `HSDLIB_NO_CHECKS` disables floating-point checks. SIMD loops must
handle short vectors and remaining elements without reading past the input buffers. Unsupported
instructions must stay behind the appropriate compile-time and runtime guards.

## Development and Validation

Use `make help` for the complete Make target list. The Nix shell provides Zig 0.17.0, the C toolchain,
analysis and documentation tools, Python, and uv. For Python work, run `uv sync --extra dev` inside the shell to
install the project's runtime and development dependencies into `.venv`.

Run the checks relevant to the files changed:

| Area             | Command                                   | Purpose                                                       |
|------------------|-------------------------------------------|---------------------------------------------------------------|
| C build          | `make build`                              | Static and shared libraries                                   |
| C tests          | `make test`                               | C suite with automatic backend selection                      |
| Zig build        | `zig build`                               | Static and shared libraries, and header installation           |
| Zig tests        | `zig build test -Dcpu=baseline`            | C suite with a baseline CPU target                             |
| Zig example      | `zig build example`                       | C example                                                     |
| Scalar tests     | `HSD_TEST_FORCE_BACKEND=SCALAR make test` | C suite with scalar dispatch                                  |
| AMD64 backends   | `make test-amd64`                         | Backend sweep on an AMD64 host                                |
| AArch64 backends | `make test-aarch64`                       | Backend sweep on an AArch64 host                              |
| C formatting     | `make format`                             | In-place formatting of `src/`, `include/`, and `tests/`       |
| C analysis       | `make lint`                               | cppcheck over C sources, headers, and tests                   |
| Python tests     | `make python-test`                        | Native rebuild, wheel build, and pytest with coverage         |
| Python lint      | `uv run ruff check bindings/python`       | Ruff checks                                                   |
| Python typing    | `uv run pyright`                          | Configured binding and test type checks                       |
| Examples         | `make example`                            | C and Python examples, including wheel build and installation |
| Benchmarks       | `make bench`                              | Backend benchmarks for the host architecture                  |
| Coverage         | `make cov`                                | C tests with gcov output under `docs/coverage/`               |
| Documentation    | `make doc`                                | Doxygen output                                                |
| Nix formatting   | `nix fmt`                                 | Nix formatting                                                |
| Nix evaluation   | `nix flake check`                         | Flake output validation                                       |

For C changes, run `make format`, `make test`, and `make lint`. Add scalar tests and the host backend
sweep for changes to metrics or dispatch. Run Python tests when changing bindings or their C API,
examples when changing public behavior, and benchmarks when changing performance-sensitive code.
Update Doxygen comments and run `make doc` for public API documentation changes. Documentation-only
changes do not require C formatting or a full test run.

`make clean`, `make rebuild`, `make python-build`, and benchmark builds remove generated artifacts,
including package-local native libraries. Do not run these concurrently with other builds or tests.
Zig builds are independent of Make artifacts. Use Zig 0.17.0 and `zig fmt --check build.zig build.zig.zon`
when changing that build. Run baseline tests on AMD64 and AArch64, and check an AArch64 cross build.
There are no dedicated `asan`, `ubsan`, or `memcheck` Make targets. For memory or
SIMD buffer changes, use an instrumented compiler build or Valgrind and report the exact command used.

## Testing Expectations

- Regression coverage for changed public behavior in the corresponding `tests/test_*.c` file, using
  existing helpers from `tests/test_common.h` and registration in `tests/main.c` when needed.
- Scalar reference comparisons and suitable floating-point tolerances for numerical changes.
- Relevant edge cases: empty vectors, null pointers, NaN and infinity, zero vectors, unaligned inputs,
  and lengths around SIMD lane boundaries.
- Python coverage in `bindings/python/tests/` for binding, dtype, shape, and error-handling changes.
- Host architecture and forced backend recorded with validation results. A forced backend can fall
  back to scalar, so a passing sweep alone does not prove execution of every SIMD implementation.

## Commit and PR Hygiene

- Keep commits scoped to one logical change.
- Include the behavioral change, regression coverage, and validation commands in PR descriptions.
- State checks that could not run, including unavailable architectures or CPU features.
- Keep generated binaries, coverage reports, and documentation output out of source changes unless
  explicitly requested.
