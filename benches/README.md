## Benchmarks

| Directory | Description               |
|-----------|---------------------------|
| [`c`](c/) | Benchmarks for Hsdlib API |

### Sample Benchmark Results for Hsdlib API

Sample benchmark results for different SIMD backends on AMD64 and AArch64 CPUs are shown in the tables below.
Benchmarks have the following parameters:

- `VECTOR_DIM`: 1536 (dimension of the vectors)
- `NUM_ITERATIONS`: 1,000,000 (number of vector pairs)

Vectors were randomly generated using the `rand()` function from the C standard library.
Distances and similarities were calculated sequentially, and benchmark harness is calculated as the average time taken
to compute the distance (or similarity) for all vector pairs.
Check out `c/bench_common.h` for the implementation details.

All benchmarks are built using GCC 13.3 compiler with `-O3` optimization level and run on Ubuntu 22.04 LTS.
Values are runtime in seconds and lower is better.

### AMD64 Benchmark

The benchmark results are from a successful run of this
[GitHub Actions workflow](https://github.com/habedi/hsdlib/actions/workflows/benches_amd64.yml).
The CPU supported AVX and AVX2 instruction sets.

| Backend         | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|-----------------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO            | 0.239833         | 0.199827      | 0.041563         | 0.915258          | 0.171770            | 0.201329              |
| SCALAR          | 1.925597         | 1.921322      | 0.975243         | 1.444249          | 1.927470            | 1.927175              |
| AVX             | 0.239686         | 0.199611      | 0.963140         | 1.440981          | 0.164513            | 0.201073              |
| AVX2            | 0.239715         | 0.199688      | 0.041879         | 0.913405          | 0.171899            | 0.201232              |
| AVX512F         | 1.926606         | 1.920597      | 0.985374         | 1.440770          | 1.928520            | 1.926965              |
| AVX512BW        | 1.931330         | 1.923627      | 0.965430         | 1.444396          | 1.924500            | 1.928394              |
| AVX512DQ        | 1.927881         | 1.920686      | 0.964877         | 1.441004          | 1.929156            | 1.926730              |
| AVX512VPOPCNTDQ | 1.927855         | 1.919431      | 0.963020         | 1.440446          | 1.941298            | 1.930729              |

### AArch64 Benchmark

The benchmark results are from a successful run of this
[GitHub Actions workflow](https://github.com/habedi/hsdlib/actions/workflows/benches_aarch64.yml).
The CPU supported NEON and SVE instruction sets.

| Backend | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|---------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO    | 0.268867         | 0.228718      | 0.085440         | 0.464404          | 0.231383            | 2.254050              |
| SCALAR  | 3.167991         | 2.039103      | 0.452891         | 0.182157          | 1.814263            | 2.254129              |
| NEON    | 0.248382         | 0.220118      | 0.452903         | 0.182190          | 0.223669            | 2.254100              |
| SVE     | 0.268909         | 0.228792      | 0.452930         | 0.182751          | 0.231655            | 2.254180              |
