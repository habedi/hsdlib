#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

/* Default backend used when HSD_BENCH_FORCE_BACKEND is unset */
#ifndef DEFAULT_BENCH_BACKEND
#define DEFAULT_BENCH_BACKEND HSD_BACKEND_AUTO
#endif

#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "hsdlib.h"

#ifndef VECTOR_DIM
#define VECTOR_DIM 1536
#endif

#ifndef NUM_ITERATIONS
#define NUM_ITERATIONS 1000000
#endif

#ifndef RANDOM_SEED
#define RANDOM_SEED (time(NULL))
#endif

static inline void initialize_random_seed(void) { srand(RANDOM_SEED); }

static inline double get_time_sec(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        exit(EXIT_FAILURE);
    }
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

static HSD_Backend parse_backend(const char *s) {
    if (!s) return DEFAULT_BENCH_BACKEND;
    if (strcmp(s, "AUTO") == 0) return HSD_BACKEND_AUTO;
    if (strcmp(s, "SCALAR") == 0) return HSD_BACKEND_SCALAR;
    if (strcmp(s, "AVX") == 0) return HSD_BACKEND_AVX;
    if (strcmp(s, "AVX2") == 0) return HSD_BACKEND_AVX2;
    if (strcmp(s, "AVX512F") == 0) return HSD_BACKEND_AVX512F;
    if (strcmp(s, "AVX512BW") == 0) return HSD_BACKEND_AVX512BW;
    if (strcmp(s, "AVX512DQ") == 0) return HSD_BACKEND_AVX512DQ;
    if (strcmp(s, "AVX512VPOPCNTDQ") == 0) return HSD_BACKEND_AVX512VPOPCNTDQ;
    if (strcmp(s, "NEON") == 0) return HSD_BACKEND_NEON;
    if (strcmp(s, "SVE") == 0) return HSD_BACKEND_SVE;
    return DEFAULT_BENCH_BACKEND;
}

static inline void generate_random_f32(float *v, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        v[i] = ((float)rand() / (RAND_MAX / 2.0f)) - 1.0f;
        if (isnan(v[i]) || isinf(v[i])) v[i] = 0.0f;
    }
}

static inline void generate_random_u8(uint8_t *v, size_t n) {
    for (size_t i = 0; i < n; ++i) v[i] = (uint8_t)(rand() % 256);
}

static inline void generate_random_u16(uint16_t *v, size_t n) {
    for (size_t i = 0; i < n; ++i) v[i] = (uint16_t)(rand() % 2);
}

#define RUN_BENCHMARK(fn, suf, dt, rt, fmt, gen, hsd_fn)                                           \
    int main(void) {                                                                               \
        /* pick backend: env overrides compile-time default */                                     \
        const char *fb = getenv("HSD_BENCH_FORCE_BACKEND");                                        \
        hsd_set_manual_backend(parse_backend(fb));                                                 \
                                                                                                   \
        initialize_random_seed();                                                                  \
        printf("Benchmarking %s_%s\n", #fn, #suf);                                                 \
        printf("Backend in use: %s\n", hsd_get_backend());                                         \
        printf("Vector dim: %d, num iterations: %d, rand seed: %d\n", VECTOR_DIM, NUM_ITERATIONS, \
               RANDOM_SEED);                                                                       \
                                                                                                   \
        size_t sz = VECTOR_DIM * sizeof(dt);                                                       \
        dt *a = malloc(sz), *b = malloc(sz);                                                       \
        if (!a || !b) {                                                                            \
            fprintf(stderr, "alloc failed\n");                                                     \
            return 1;                                                                              \
        }                                                                                          \
        gen(a, VECTOR_DIM);                                                                        \
        gen(b, VECTOR_DIM);                                                                        \
                                                                                                   \
        volatile rt result;                                                                        \
        hsd_status_t st;                                                                           \
        double t0, t1;                                                                             \
                                                                                                   \
        /* warm-up */                                                                              \
        st = hsd_fn(a, b, VECTOR_DIM, (rt *)&result);                                              \
        if (st != HSD_SUCCESS) fprintf(stderr, "warm-up failed: %d\n", st);                        \
                                                                                                   \
        t0 = get_time_sec();                                                                       \
        for (int i = 0; i < NUM_ITERATIONS; ++i) {                                                 \
            st = hsd_fn(a, b, VECTOR_DIM, (rt *)&result);                                          \
            if (st != HSD_SUCCESS) {                                                               \
                fprintf(stderr, "iter %d failed: %d\n", i, st);                                    \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
        t1 = get_time_sec();                                                                       \
                                                                                                   \
        double total = t1 - t0;                                                                    \
        double per_iter = total / NUM_ITERATIONS;                                                  \
        double ops_sec = NUM_ITERATIONS / total;                                                   \
                                                                                                   \
        printf("Last result: " fmt "\n", result);                                                  \
        printf("Total time: %.5f s\n", total);                                                     \
        printf("Time per iter: %.9f s\n", per_iter);                                               \
        printf("Ops/sec: %.2f\n", ops_sec);                                                        \
                                                                                                   \
        free(a);                                                                                   \
        free(b);                                                                                   \
        return 0;                                                                                  \
    }

#endif  // BENCH_COMMON_H
