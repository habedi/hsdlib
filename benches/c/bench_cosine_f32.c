#include "bench_common.h"

RUN_BENCHMARK(cosine, f32, float, float, "%f", generate_random_f32, hsd_sim_cosine_f32)
