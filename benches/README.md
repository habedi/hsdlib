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
| AUTO            | 0.23977          | 0.22040       | 0.04192          | 0.91230           | 0.16509             | 0.22214               |
| SCALAR          | 1.97680          | 1.92111       | 0.76417          | 1.45352           | 1.91891             | 1.94650               |
| AVX             | 0.24033          | 0.19967       | 0.76271          | 1.44779           | 0.16449             | 0.20133               |
| AVX2            | 0.24012          | 0.22042       | 0.04193          | 0.91202           | 0.16935             | 0.22407               |
| AVX512F         | 1.92577          | 1.91934       | 0.76340          | 1.44261           | 1.93602             | 1.93231               |
| AVX512BW        | 1.92604          | 1.92125       | 0.75502          | 1.44157           | 1.91425             | 1.91481               |
| AVX512DQ        | 1.92561          | 1.91634       | 0.76431          | 1.44308           | 1.92608             | 1.91666               |
| AVX512VPOPCNTDQ | 1.92707          | 1.91609       | 0.76773          | 1.44231           | 1.91775             | 1.91852               |

The results below are from a successful run on an AMD64 CPU that supported AVX512 instruction set.

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

### AArch64 Benchmarks

The benchmark results are from a successful run of this
[GitHub Actions workflow](https://github.com/habedi/hsdlib/actions/workflows/benches_aarch64.yml)
on a runner with a CPU that supported NEON and SVE instruction sets.

| Backend | bench_cosine_f32 | bench_dot_f32 | bench_hamming_u8 | bench_jaccard_u16 | bench_manhattan_f32 | bench_sqeuclidean_f32 |
|---------|------------------|---------------|------------------|-------------------|---------------------|-----------------------|
| AUTO    | 0.26893          | 0.22748       | 0.17876          | 1.25252           | 0.23150             | 0.23081               |
| SCALAR  | 3.16565          | 2.03903       | 0.45297          | 0.18185           | 1.81436             | 2.25408               |
| NEON    | 0.24836          | 0.21956       | 0.08548          | 0.47345           | 0.22372             | 0.22436               |
| SVE     | 0.26896          | 0.22746       | 0.17866          | 1.24904           | 0.23142             | 0.23053               |
