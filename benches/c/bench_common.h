#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "hsdlib.h"

#ifndef VECTOR_DIM
#define VECTOR_DIM 1536
#endif

#ifndef NUM_ITERATIONS
#define NUM_ITERATIONS 100000
#endif

#ifndef RANDOM_SEED
#define RANDOM_SEED (time(NULL))
#endif

static inline void initialize_random_seed() {
    srand(RANDOM_SEED);
}

static inline double get_time_sec() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static inline void generate_random_f32(float *vec, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        vec[i] = ((float)rand() / (float)(RAND_MAX / 2.0f)) - 1.0f;
        if (isnan(vec[i]) || isinf(vec[i])) {
            vec[i] = 0.0f;
        }
    }
}

static inline void generate_random_u8(uint8_t *vec, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        vec[i] = (uint8_t)(rand() % 256);
    }
}

static inline void generate_random_u16(uint16_t *vec, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        vec[i] = (uint16_t)(rand() % 2);
    }
}

#define RUN_BENCHMARK(func_name, type_suffix, data_type, result_type, result_format, generate_func, hsd_func) \
    int main(void) {                                           \
        initialize_random_seed();                              \
        const char* backend_name = hsd_get_backend();          \
        printf("Benchmarking %s_%s\n", #func_name, #type_suffix);                \
        printf("Backend detected: %s\n", backend_name);                          \
        printf("Vector Dimension: %d\n", VECTOR_DIM);                            \
        printf("Iterations: %d\n", NUM_ITERATIONS);                              \
                                                                                 \
        size_t data_size = VECTOR_DIM * sizeof(data_type);                       \
        data_type *a = (data_type *)malloc(data_size);                           \
        data_type *b = (data_type *)malloc(data_size);                           \
                                                                                 \
        if (!a || !b) {                                                          \
            fprintf(stderr, "Memory allocation failed\n");                       \
            return 1;                                                            \
        }                                                                        \
                                                                                 \
        generate_func(a, VECTOR_DIM);                                            \
        generate_func(b, VECTOR_DIM);                                            \
                                                                                 \
        volatile result_type result;                                             \
        hsd_status_t status;                                                     \
        double start_time, end_time, total_time;                                 \
                                                                                 \
        status = hsd_func(a, b, VECTOR_DIM, (result_type *)&result);             \
        if (status != HSD_SUCCESS) {                                             \
             fprintf(stderr, "Warm-up call failed with status: %d\n", status);   \
        }                                                                        \
                                                                                 \
        start_time = get_time_sec();                                             \
        for (int i = 0; i < NUM_ITERATIONS; ++i) {                               \
            status = hsd_func(a, b, VECTOR_DIM, (result_type *)&result);         \
            if (status != HSD_SUCCESS) {                                         \
                fprintf(stderr, "Bench call %d failed with status: %d\n", i, status); \
                free(a); free(b); return 1;                                      \
            }                                                                    \
        }                                                                        \
        end_time = get_time_sec();                                               \
                                                                                 \
        total_time = end_time - start_time;                                      \
        double time_per_iteration = total_time / NUM_ITERATIONS;                 \
        double ops_per_sec = NUM_ITERATIONS / total_time;                        \
                                                                                 \
        printf("Last result: " result_format "\n", result);                      \
        printf("Total time: %.6f seconds\n", total_time);                        \
        printf("Time per iteration: %.9f seconds (%.3f ms)\n",                   \
               time_per_iteration, time_per_iteration * 1000.0);                 \
        printf("Operations per second: %.2f\n", ops_per_sec);                    \
                                                                                 \
        free(a);                                                                 \
        free(b);                                                                 \
                                                                                 \
        return 0;                                                                \
    }

#endif // BENCH_COMMON_H
