#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hsdlib.h"
#include "test_common.h"

static int compare_vectors_f32(const float* expected, const float* actual, size_t n,
                               float tolerance, const char* test_name) {
    int match = 1;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(expected[i]) && isnan(actual[i])) {
            continue;
        }

        if (isinf(expected[i]) && isinf(actual[i]) &&
            (signbit(expected[i]) == signbit(actual[i]))) {
            continue;
        }

        if (fabsf(expected[i] - actual[i]) > tolerance) {
            if (match) {
                fprintf(stderr, "FAIL: Vector mismatch in test '%s'\n", test_name);
            }
            fprintf(stderr, "      Mismatch at index %zu: Expected %.8f, Actual %.8f, Diff %.8e\n",
                    i, expected[i], actual[i], fabsf(expected[i] - actual[i]));
            match = 0;
            g_test_failed++;
        }
    }
    if (!match) {
    }
    return match;
}

static void simple_normalize_l2(const float* input, size_t n, float* expected_output) {
    if (n == 0) return;

    double sum_sq = 0.0;
    int invalid_input = 0;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(input[i]) || isinf(input[i])) {
            invalid_input = 1;
            break;
        }
        sum_sq += (double)input[i] * (double)input[i];
    }

    if (invalid_input || isnan(sum_sq) || isinf(sum_sq)) {
        for (size_t j = 0; j < n; ++j) expected_output[j] = NAN;
        return;
    }

    double norm = sqrt(sum_sq);

    if (isnan(norm) || norm <= 0.0) {
        memcpy(expected_output, input, n * sizeof(float));
        return;
    }

    double inv_norm = 1.0 / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) {
        for (size_t j = 0; j < n; ++j) expected_output[j] = NAN;
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        expected_output[i] = (float)((double)input[i] * inv_norm);
        if (isnan(expected_output[i])) {
            for (size_t j = 0; j < n; ++j) expected_output[j] = NAN;
            break;
        }
    }
}

static void run_normalize_test(const char* test_name, float* input_vec, size_t n,
                               hsd_status_t expected_status, float tolerance) {
    printf("-- Running test: %s (n=%zu) --\n", test_name, n);

    float* expected_vec = NULL;
    float* actual_vec = (float*)malloc(n * sizeof(float));
    if (n > 0) {
        expected_vec = (float*)malloc(n * sizeof(float));
    }

    if ((n > 0 && !expected_vec) || !actual_vec) {
        fprintf(stderr, "FAIL: Memory allocation failed for test '%s'\n", test_name);
        g_test_failed++;
        free(expected_vec);
        free(actual_vec);
        printf("\n");
        return;
    }

    if (n > 0) {
        memcpy(actual_vec, input_vec, n * sizeof(float));
        simple_normalize_l2(input_vec, n, expected_vec);
    }

    hsd_status_t actual_status = hsd_normalize_l2_f32(actual_vec, n);

    if (actual_status != expected_status) {
        fprintf(stderr, "FAIL: %s - Expected status %d, but got %d\n", test_name, expected_status,
                actual_status);
        g_test_failed++;
    } else {
        if (actual_status == HSD_SUCCESS) {
            if (compare_vectors_f32(expected_vec, actual_vec, n, tolerance, test_name)) {
                printf("PASS: %s (Status %d, Vectors match within tolerance %.2e)\n", test_name,
                       actual_status, tolerance);
            } else {
            }
        } else {
            printf("PASS: %s (Correctly returned expected failure status %d)\n", test_name,
                   actual_status);
        }
    }

    free(expected_vec);
    free(actual_vec);
    printf("\n");
}

void run_vector_ops_tests(void) {
    printf("\n======= Running Vector Ops Tests (Normalize L2) =======\n");

    float vec1[] = {3.0f, 4.0f, 0.0f};
    run_normalize_test("Basic 3D", vec1, 3, HSD_SUCCESS, 1e-7f);

    float vec_neg[] = {-1.0f, -2.0f, 2.0f};
    run_normalize_test("Negative Coords", vec_neg, 3, HSD_SUCCESS, 1e-7f);

    float vec_zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
    run_normalize_test("Zero Vector", vec_zero, 4, HSD_SUCCESS, 1e-7f);

    float vec_already[] = {0.6f, -0.8f};
    run_normalize_test("Already Normalized", vec_already, 2, HSD_SUCCESS, 1e-7f);

    float vec_empty[1];
    run_normalize_test("Zero Dimension", vec_empty, 0, HSD_SUCCESS, 1e-7f);

    float vec_one[] = {-5.0f};
    run_normalize_test("One Dimension", vec_one, 1, HSD_SUCCESS, 1e-7f);

    float v7[7];
    for (int i = 0; i < 7; ++i) v7[i] = (float)i + 1;
    run_normalize_test("Dimension 7", v7, 7, HSD_SUCCESS, 1e-6f);

    float v8[8];
    for (int i = 0; i < 8; ++i) v8[i] = (float)i + 1;
    run_normalize_test("Dimension 8", v8, 8, HSD_SUCCESS, 1e-6f);

    float v9[9];
    for (int i = 0; i < 9; ++i) v9[i] = (float)i + 1;
    run_normalize_test("Dimension 9", v9, 9, HSD_SUCCESS, 1e-6f);

    float v15[15];
    for (int i = 0; i < 15; ++i) v15[i] = (float)i + 1;
    run_normalize_test("Dimension 15", v15, 15, HSD_SUCCESS, 1e-6f);

    float v16[16];
    for (int i = 0; i < 16; ++i) v16[i] = (float)i + 1;
    run_normalize_test("Dimension 16", v16, 16, HSD_SUCCESS, 1e-6f);

    float v17[17];
    for (int i = 0; i < 17; ++i) v17[i] = (float)i + 1;
    run_normalize_test("Dimension 17", v17, 17, HSD_SUCCESS, 1e-6f);

    float vec_small[] = {1e-20f, 1e-20f};
    run_normalize_test("Small Values (Subnormal Norm)", vec_small, 2, HSD_SUCCESS, 2e-6f);

    float vec_large[] = {FLT_MAX / 2.0f, FLT_MAX / 2.0f};
    run_normalize_test("Large Values", vec_large, 2, HSD_FAILURE, 1e-7f);

    float vec_max[] = {FLT_MAX, 0.0f};
    run_normalize_test("Max Float Value", vec_max, 2, HSD_FAILURE, 1e-7f);

    float vec_nan[] = {1.0f, NAN, 3.0f};
    run_normalize_test("NaN Input", vec_nan, 3, HSD_FAILURE, 1e-7f);

    float vec_inf[] = {1.0f, INFINITY, 3.0f};
    run_normalize_test("Inf Input", vec_inf, 3, HSD_FAILURE, 1e-7f);

    float vec_overflow_sumsq[2] = {FLT_MAX, FLT_MAX};
    run_normalize_test("Overflow Norm", vec_overflow_sumsq, 2, HSD_FAILURE, 1e-7f);

    printf("-- Running test: NULL Pointer Input --\n");
    hsd_status_t status = hsd_normalize_l2_f32(NULL, 5);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer Input (Correctly returned status %d)\n", status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer Input\n");
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    printf("======= Finished Vector Ops Tests (Normalize L2) =======\n");
}
