#include "bench_common.h"

RUN_BENCHMARK(jaccard, u16, uint16_t, float, "%f", generate_random_u16, hsd_sim_jaccard_u16)
