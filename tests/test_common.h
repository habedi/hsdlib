#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "hsdlib.h"

extern int g_test_failed;

typedef hsd_status_t (*hsd_func_f32_f32)(const float *a, const float *b, size_t n, float *result);
typedef hsd_status_t (*hsd_func_u8_u64)(const uint8_t *a, const uint8_t *b, size_t n,
                                        uint64_t *result);
typedef hsd_status_t (*hsd_func_u16_f32)(const uint16_t *a, const uint16_t *b, size_t n,
                                         float *result);

void run_test_f32(hsd_func_f32_f32 func_to_test, const char *func_name_str, const char *test_name,
                  const float *a, const float *b, size_t n, float expected_result, float tolerance);

void run_test_u64_u8_input(hsd_func_u8_u64 func_to_test, const char *func_name_str,
                           const char *test_name, const uint8_t *a, const uint8_t *b, size_t n,
                           uint64_t expected_result);

void run_test_f32_u16_input(hsd_func_u16_f32 func_to_test, const char *func_name_str,
                            const char *test_name, const uint16_t *a, const uint16_t *b, size_t n,
                            float expected_result, float tolerance);

void run_test_expect_failure_status_f32(hsd_func_f32_f32 func_to_test, const char *func_name_str,
                                        const char *test_name, const float *a, const float *b,
                                        size_t n);
void run_test_expect_failure_status_u8(hsd_func_u8_u64 func_to_test, const char *func_name_str,
                                       const char *test_name, const uint8_t *a, const uint8_t *b,
                                       size_t n);
void run_test_expect_failure_status_u16(hsd_func_u16_f32 func_to_test, const char *func_name_str,
                                        const char *test_name, const uint16_t *a, const uint16_t *b,
                                        size_t n);

float simple_sqeuclidean_f32(const float *a, const float *b, const size_t n);
float simple_cosine_sim_f32(const float *a, const float *b, const size_t n);
float simple_dot_f32(const float *a, const float *b, const size_t n);
float simple_manhattan_f32(const float *a, const float *b, const size_t n);
uint64_t simple_hamming_u8(const uint8_t *a, const uint8_t *b, const size_t n);
float simple_jaccard_sim_u16(const uint16_t *a, const uint16_t *b, const size_t n);

#endif
