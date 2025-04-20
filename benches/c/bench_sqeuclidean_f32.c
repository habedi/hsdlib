#include "bench_common.h"

RUN_BENCHMARK(sqeuclidean, f32, float, float, "%f", generate_random_f32, hsd_dist_sqeuclidean_f32)
