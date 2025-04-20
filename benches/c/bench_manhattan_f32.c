#include "bench_common.h"

RUN_BENCHMARK(manhattan, f32, float, float, "%f", generate_random_f32, hsd_dist_manhattan_f32)
