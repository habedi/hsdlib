## Benchmarks

| Directory | Description               |
|-----------|---------------------------|
| [`c`](c/) | Benchmarks for Hsdlib API |

### Sample Benchmark Results for Hsdlib API

Sample benchmark results for different SIMD backends on AMD64 and AArch64 CPUs are shown in the tables below.
Benchmarks have the following parameters:

- `VECTOR_DIM`: 1536 (dimension of the vectors)
- `NUM_ITERATIONS`: 1,000,000 (number of vector pairs)
- `RANDOM_SEED`: random number generator seed (in [Makefile](../Makefile) 53 is used for seed)

Vectors were randomly generated using the `rand()` function from the C standard library.
Distances and similarities were calculated sequentially, and benchmark harness is calculated as the average time taken
to compute the distance (or similarity) for all vector pairs.
Check out `c/bench_common.h` for the full details how the benchmarks are implemented.

All benchmarks are built using GCC (13.3) with `-O3` optimization level and run on Ubuntu 22.04 LTS or later.
Values are runtime in seconds and lower is better.

### AMD64 Benchmarks

The benchmark results are from a successful run of this
[GitHub Actions workflow](https://github.com/habedi/hsdlib/actions/workflows/benches_amd64.yml)
on a runner with a CPU that supported AVX and AVX2 instruction sets.

| Backend         | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|-----------------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO            | 0.239729         | 0.199880      | 0.041618         | 0.915996          | 0.171932            | 0.201219              |
| SCALAR          | 1.927583         | 1.927635      | 0.964101         | 1.443555          | 1.941286            | 1.951248              |
| AVX             | 0.239655         | 0.199585      | 0.964768         | 1.440620          | 0.164594            | 0.201132              |
| AVX2            | 0.239908         | 0.200492      | 0.041900         | 0.915071          | 0.172088            | 0.201194              |
| AVX512F         | 1.925864         | 1.923918      | 0.963930         | 1.440904          | 1.924354            | 1.930401              |
| AVX512BW        | 1.926156         | 1.923771      | 0.973348         | 1.442347          | 1.925261            | 1.925575              |
| AVX512DQ        | 1.926762         | 1.920748      | 0.964245         | 1.440104          | 1.924370            | 1.926917              |
| AVX512VPOPCNTDQ | 1.928641         | 1.921946      | 0.964263         | 1.441354          | 1.926447            | 1.928536              |

| Backend         | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|-----------------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO            | 0.089076         | 0.054583      | 0.014012         | 0.308124          | 0.054883            | 0.056979              |
| SCALAR          | 1.186225         | 1.172793      | 0.445971         | 0.865517          | 1.171607            | 1.171269              |
| AVX             | 0.144463         | 0.121932      | 0.443385         | 0.865236          | 0.097709            | 0.121355              |
| AVX2            | 0.148614         | 0.109022      | 0.025315         | 0.295954          | 0.094523            | 0.112432              |
| AVX512F         | 0.078457         | 0.053045      | 0.442175         | 0.869115          | 0.054395            | 0.056841              |
| AVX512BW        | 1.183811         | 1.174641      | 0.447524         | 0.264160          | 1.164908            | 1.165508              |
| AVX512DQ        | 1.195070         | 1.168193      | 0.444074         | 0.264640          | 1.180334            | 1.171049              |
| AVX512VPOPCNTDQ | 1.188696         | 1.173643      | 0.013351         | 0.870104          | 1.169641            | 1.168621              |

### AArch64 Benchmark

The benchmark results are from a successful run of this
[GitHub Actions workflow](https://github.com/habedi/hsdlib/actions/workflows/benches_aarch64.yml)
on a runner with a CPU that supported NEON and SVE instruction sets.

| Backend | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|---------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO    | 0.268753         | 0.228842      | 0.114529         | 0.718648          | 0.231373            | 0.230661              |
| SCALAR  | 3.165892         | 2.039137      | 0.452927         | 0.182411          | 1.814347            | 2.254057              |
| NEON    | 0.248283         | 0.220993      | 0.085842         | 0.464017          | 0.226559            | 0.226573              |
| SVE     | 0.268889         | 0.228794      | 0.114476         | 0.718046          | 0.231360            | 0.230642              |
