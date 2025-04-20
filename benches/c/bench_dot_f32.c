#include "bench_common.h"

RUN_BENCHMARK(dot, f32, float, float, "%f", generate_random_f32, hsd_sim_dot_f32)
