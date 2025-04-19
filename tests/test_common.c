#include "test_common.h"

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_test_failed = 0;

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

static inline uint8_t simple_popcount8(uint8_t val) {
#if __has_builtin(__builtin_popcount)
    return (uint8_t)__builtin_popcount(val);
#else
    uint8_t count = 0;
    while (val > 0) {
        val &= (val - 1);
        count++;
    }
    return count;
#endif
}

static void check_float_result(const char *test_name, const char *func_name_str,
                               float expected_result, float actual_result, float tolerance) {
    int comparison_ok = 0;

    if (isinf(expected_result)) {
        if (isinf(actual_result) && (signbit(expected_result) == signbit(actual_result))) {
            comparison_ok = 1;
        }
    } else if (isnan(expected_result)) {
        if (isnan(actual_result)) {
            comparison_ok = 1;
            printf("WARN: %s [%s] - Expected and Actual are NaN.\n", test_name, func_name_str);
        }
    } else if (!isnan(actual_result) && !isinf(actual_result)) {
        comparison_ok = (fabsf(expected_result - actual_result) <= tolerance);
    }

    if (comparison_ok) {
        printf("PASS: %s [%s] (Expected: %.8f, Actual: %.8f)\n", test_name, func_name_str,
               expected_result, actual_result);
    } else {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Expected: %.8f\n", expected_result);
        fprintf(stderr, "      Actual:   %.8f\n", actual_result);

        if (!isnan(expected_result) && !isinf(expected_result) && !isnan(actual_result) &&
            !isinf(actual_result)) {
            fprintf(stderr, "      Difference: %.8e > Tolerance: %.8e\n",
                    fabsf(expected_result - actual_result), tolerance);
        }
        g_test_failed++;
    }
}

static void check_u64_result(const char *test_name, const char *func_name_str,
                             uint64_t expected_result, uint64_t actual_result) {
    if (expected_result == actual_result) {
        printf("PASS: %s [%s] (Expected: %" PRIu64 ", Actual: %" PRIu64 ")\n", test_name,
               func_name_str, expected_result, actual_result);
    } else {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Expected: %" PRIu64 "\n", expected_result);
        fprintf(stderr, "      Actual:   %" PRIu64 "\n", actual_result);
        g_test_failed++;
    }
}

void run_test_f32(hsd_func_f32_f32 func_to_test, const char *func_name_str, const char *test_name,
                  const float *a, const float *b, size_t n, float expected_result,
                  float tolerance) {
    printf("-- Running test: %s [%s] (n=%zu) --\n", test_name, func_name_str, n);
    hsd_log("Test setup: expected=%.8f, tolerance=%.8e", expected_result, tolerance);

    float actual_result = -999.0f;
    hsd_status_t status = func_to_test(a, b, n, &actual_result);

    if (status != HSD_SUCCESS) {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Function unexpectedly returned status %d\n", status);
        g_test_failed++;
        printf("\n");
        return;
    }

    check_float_result(test_name, func_name_str, expected_result, actual_result, tolerance);
    printf("\n");
}

void run_test_u64_u8_input(hsd_func_u8_u64 func_to_test, const char *func_name_str,
                           const char *test_name, const uint8_t *a, const uint8_t *b, size_t n,
                           uint64_t expected_result) {
    printf("-- Running test: %s [%s] (n=%zu) --\n", test_name, func_name_str, n);
    hsd_log("Test setup: expected=%" PRIu64, expected_result);

    uint64_t actual_result = UINT64_MAX;
    hsd_status_t status = func_to_test(a, b, n, &actual_result);

    if (status != HSD_SUCCESS) {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Function unexpectedly returned status %d\n", status);
        g_test_failed++;
        printf("\n");
        return;
    }

    check_u64_result(test_name, func_name_str, expected_result, actual_result);
    printf("\n");
}

void run_test_f32_u16_input(hsd_func_u16_f32 func_to_test, const char *func_name_str,
                            const char *test_name, const uint16_t *a, const uint16_t *b, size_t n,
                            float expected_result, float tolerance) {
    printf("-- Running test: %s [%s] (n=%zu) --\n", test_name, func_name_str, n);
    hsd_log("Test setup: expected=%.8f, tolerance=%.8e", expected_result, tolerance);

    float actual_result = -999.0f;
    hsd_status_t status = func_to_test(a, b, n, &actual_result);

    if (status != HSD_SUCCESS) {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Function unexpectedly returned status %d\n", status);
        g_test_failed++;
        printf("\n");
        return;
    }

    check_float_result(test_name, func_name_str, expected_result, actual_result, tolerance);
    printf("\n");
}

static void run_test_expect_failure_generic(const char *func_name_str, const char *test_name,
                                            size_t n, hsd_status_t actual_status) {
    printf("-- Running test: %s [%s] (n=%zu) --\n", test_name, func_name_str, n);
    hsd_log("Test setup: Expecting non-HSD_SUCCESS status");

    if (actual_status != HSD_SUCCESS) {
        printf("PASS: %s [%s] (Correctly returned non-success status %d)\n", test_name,
               func_name_str, actual_status);
    } else {
        fprintf(stderr, "FAIL: %s [%s]\n", test_name, func_name_str);
        fprintf(stderr, "      Expected non-success status, but got %d (HSD_SUCCESS)\n",
                actual_status);
        g_test_failed++;
    }
    printf("\n");
}

void run_test_expect_failure_status_f32(hsd_func_f32_f32 func_to_test, const char *func_name_str,
                                        const char *test_name, const float *a, const float *b,
                                        size_t n) {
    float dummy_result = -999.0f;
    hsd_status_t status = func_to_test(a, b, n, &dummy_result);
    run_test_expect_failure_generic(func_name_str, test_name, n, status);
}

void run_test_expect_failure_status_u8(hsd_func_u8_u64 func_to_test, const char *func_name_str,
                                       const char *test_name, const uint8_t *a, const uint8_t *b,
                                       size_t n) {
    uint64_t dummy_result = UINT64_MAX;
    hsd_status_t status = func_to_test(a, b, n, &dummy_result);
    run_test_expect_failure_generic(func_name_str, test_name, n, status);
}

void run_test_expect_failure_status_u16(hsd_func_u16_f32 func_to_test, const char *func_name_str,
                                        const char *test_name, const uint16_t *a, const uint16_t *b,
                                        size_t n) {
    float dummy_result = -999.0f;
    hsd_status_t status = func_to_test(a, b, n, &dummy_result);
    run_test_expect_failure_generic(func_name_str, test_name, n, status);
}

float simple_sqeuclidean_f32(const float *a, const float *b, const size_t n) {
    if (n == 0) return 0.0f;
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) return NAN;
        const double diff = (double)a[i] - (double)b[i];
        sum += diff * diff;
    }

    if (sum > (double)FLT_MAX) return INFINITY;
    if (isinf(sum)) return INFINITY;
    if (isnan(sum)) return NAN;

    return (float)sum;
}

float simple_cosine_sim_f32(const float *a, const float *b, const size_t n) {
    if (n == 0) return 1.0f;  // Define similarity of zero-length vectors as 1.0

    double dot_product = 0.0;
    double norm_a_sq = 0.0;
    double norm_b_sq = 0.0;

    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) return NAN;
        dot_product += (double)a[i] * (double)b[i];
        norm_a_sq += (double)a[i] * (double)a[i];
        norm_b_sq += (double)b[i] * (double)b[i];
    }

    if (isinf(norm_a_sq) || isinf(norm_b_sq) || isinf(dot_product) || isnan(norm_a_sq) ||
        isnan(norm_b_sq) || isnan(dot_product))
        return NAN;

    int is_a_zero = !(norm_a_sq > DBL_EPSILON);
    int is_b_zero = !(norm_b_sq > DBL_EPSILON);

    if (is_a_zero && is_b_zero) return 1.0f;
    if (is_a_zero || is_b_zero) return 0.0f;

    double norm_a = sqrt(norm_a_sq);
    double norm_b = sqrt(norm_b_sq);

    if (isinf(norm_a) || isinf(norm_b) || isnan(norm_a) || isnan(norm_b) || norm_a < DBL_EPSILON ||
        norm_b < DBL_EPSILON)
        return NAN;  // Should not happen if sq norms > 0 and not Inf/NaN

    double denominator = norm_a * norm_b;

    if (denominator < DBL_MIN) {
        return 0.0f;  // Treat as orthogonal if denominator is tiny
    }

    double similarity = dot_product / denominator;

    // Clamp due to potential floating point issues
    if (similarity > 1.0) similarity = 1.0;
    if (similarity < -1.0) similarity = -1.0;

    if (isnan(similarity)) return NAN;

    return (float)similarity;
}

float simple_dot_f32(const float *a, const float *b, const size_t n) {
    if (n == 0) return 0.0f;
    double dot_product = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) return NAN;
        dot_product += (double)a[i] * (double)b[i];
    }

    if (isinf(dot_product)) return (float)dot_product;
    if (isnan(dot_product)) return NAN;

    // Check against FLT_MAX range before casting
    if (dot_product > (double)FLT_MAX) return INFINITY;
    if (dot_product < (double)-FLT_MAX) return -INFINITY;

    return (float)dot_product;
}

float simple_manhattan_f32(const float *a, const float *b, const size_t n) {
    if (n == 0) return 0.0f;
    double sum_abs_diff = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) return NAN;
        sum_abs_diff += fabs((double)a[i] - (double)b[i]);
    }

    if (sum_abs_diff > (double)FLT_MAX) return INFINITY;
    if (isinf(sum_abs_diff)) return INFINITY;
    if (isnan(sum_abs_diff)) return NAN;

    return (float)sum_abs_diff;
}

uint64_t simple_hamming_u8(const uint8_t *a, const uint8_t *b, const size_t n) {
    if (n == 0) return 0;

    uint64_t diff_count = 0;
    for (size_t i = 0; i < n; ++i) {
        diff_count += (uint64_t)simple_popcount8(a[i] ^ b[i]);
    }
    return diff_count;
}

float simple_jaccard_sim_u16(const uint16_t *a, const uint16_t *b, const size_t n) {
    if (n == 0) return 1.0f;  // Define similarity of zero-length vectors as 1.0

    uint64_t dot_p = 0;
    uint64_t n_a_sq = 0;
    uint64_t n_b_sq = 0;

    for (size_t i = 0; i < n; ++i) {
        uint64_t val_a = (uint64_t)a[i];
        uint64_t val_b = (uint64_t)b[i];
        dot_p += val_a * val_b;
        n_a_sq += val_a * val_a;
        n_b_sq += val_b * val_b;
    }

    if (n_a_sq == 0 && n_b_sq == 0) {
        return 1.0f;  // Both vectors are zero
    }

    double d_dot = (double)dot_p;
    double d_nAsq = (double)n_a_sq;
    double d_nBsq = (double)n_b_sq;
    double denominator = d_nAsq + d_nBsq - d_dot;
    double similarity;

    // Denominator == 0 implies A == B (or both zero)
    if (denominator < DBL_EPSILON) {
        similarity = 1.0;
    } else {
        similarity = d_dot / denominator;
    }

    // Clamp similarity [0.0, 1.0]
    if (similarity > 1.0) similarity = 1.0;
    if (similarity < 0.0) similarity = 0.0;  // Should not happen

    if (isnan(similarity)) return NAN;

    return (float)similarity;
}
