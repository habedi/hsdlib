#include "bench_common.h"

RUN_BENCHMARK(hamming, u8, uint8_t, uint64_t, "%lu", generate_random_u8, hsd_dist_hamming_u8)
