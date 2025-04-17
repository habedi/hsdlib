#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

void run_euclidean_tests(void) {
    printf("\n======= Running Euclidean Tests =======\n");

    hsd_func_f32_f32 func_ptr = hsd_euclidean_f32;
    const char* func_name = "hsd_euclidean_f32";

    const float vec1[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    const float vec2[] = {9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f};

    run_test_f32(func_ptr, func_name, "Original Test", vec1, vec2, 9,
                 simple_euclidean(vec1, vec2, 9), 1e-6f);

    float v_ident1[] = {1.1f, -2.2f, 3.3f, -4.4f};
    const float v_ident2[] = {1.1f, -2.2f, 3.3f, -4.4f};
    run_test_f32(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 4, 0.0f, 1e-7f);

    const float v_zero[] = {0.0f, 0.0f, 0.0f};
    const float v_340[] = {3.0f, 4.0f, 0.0f};
    run_test_f32(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero, v_340, 3, 5.0f, 1e-7f);

    const float v_neg1[] = {-1.0f, -2.0f};
    const float v_neg2[] = {-4.0f, -6.0f};
    run_test_f32(func_ptr, func_name, "Negative Coordinates", v_neg1, v_neg2, 2, 5.0f, 1e-7f);

    float v_dummy[1];
    run_test_f32(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 0.0f, 1e-7f);

    const float v1_a[] = {5.5f};
    const float v1_b[] = {-2.0f};
    run_test_f32(func_ptr, func_name, "One Dimension", v1_a, v1_b, 1, 7.5f, 1e-7f);

    const float v3_a[] = {1, 2, 3};
    const float v3_b[] = {4, 5, 6};
    run_test_f32(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3, sqrtf(27.0f), 1e-6f);

    float v7_a[] = {1, 1, 1, 1, 1, 1, 1};
    float v7_b[] = {2, 2, 2, 2, 2, 2, 2};
    run_test_f32(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7, sqrtf(7.0f), 1e-6f);
    float v8_a[] = {1, 1, 1, 1, 1, 1, 1, 1};
    float v8_b[] = {2, 2, 2, 2, 2, 2, 2, 2};
    run_test_f32(func_ptr, func_name, "Dimension 8", v8_a, v8_b, 8, sqrtf(8.0f), 1e-6f);
    float v9_a[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    float v9_b[] = {2, 2, 2, 2, 2, 2, 2, 2, 2};
    run_test_f32(func_ptr, func_name, "Dimension 9", v9_a, v9_b, 9, 3.0f, 1e-6f);
    float v15_a[15], v15_b[15];
    for (int i = 0; i < 15; ++i) {
        v15_a[i] = 1;
        v15_b[i] = 2;
    }
    run_test_f32(func_ptr, func_name, "Dimension 15", v15_a, v15_b, 15, sqrtf(15.0f), 1e-6f);
    float v16_a[16], v16_b[16];
    for (int i = 0; i < 16; ++i) {
        v16_a[i] = 1;
        v16_b[i] = 2;
    }
    run_test_f32(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16, 4.0f, 1e-6f);
    float v17_a[17], v17_b[17];
    for (int i = 0; i < 17; ++i) {
        v17_a[i] = 1;
        v17_b[i] = 2;
    }
    run_test_f32(func_ptr, func_name, "Dimension 17", v17_a, v17_b, 17, sqrtf(17.0f), 1e-6f);

    float v_small1[] = {1e-20f, 2e-20f};
    float v_small2[] = {3e-20f, 4e-20f};
    run_test_f32(func_ptr, func_name, "Small Values", v_small1, v_small2, 2,
                 simple_euclidean(v_small1, v_small2, 2), 1e-25f);

    float v_large1[] = {1e19f, 0.0f};
    float v_large2[] = {0.0f, 1e19f};
    run_test_f32(func_ptr, func_name, "Large Values", v_large1, v_large2, 2,
                 simple_euclidean(v_large1, v_large2, 2), 1e13f);

    float v_overflow1[] = {FLT_MAX / 1.5f, 0.0f};
    float v_overflow2[] = {-FLT_MAX / 1.5f, 0.0f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "Potential Overflow diff*diff",
                                       v_overflow1, v_overflow2, 2);

    float v_sum_overflow1[10];
    float v_sum_overflow2[10];
    for (int i = 0; i < 10; ++i) {
        v_sum_overflow1[i] = sqrtf(FLT_MAX / 10.0f);
        v_sum_overflow2[i] = 0.0f;
    }
    run_test_expect_failure_status_f32(func_ptr, func_name, "Potential Overflow Sum",
                                       v_sum_overflow1, v_sum_overflow2, 10);

    float v_ok[] = {1.0f, 2.0f, 3.0f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_euclidean_f32(v_ok, v_ok, 3, NULL);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer: result [%s] (Correctly returned status %d)\n", func_name,
               status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer: result [%s]\n", func_name);
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    float v_nan1[] = {1.0f, NAN, 3.0f};
    float v_nan2[] = {1.0f, 2.0f, NAN};
    run_test_expect_failure_status_f32(func_ptr, func_name, "NaN Input Vec A", v_nan1, v_ok, 3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "NaN Input Vec B", v_ok, v_nan2, 3);

    float v_inf1[] = {1.0f, INFINITY, 3.0f};
    float v_inf2[] = {-INFINITY, 2.0f, 3.0f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "Infinity Input Vec A", v_inf1, v_ok,
                                       3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "Infinity Input Vec B", v_ok, v_inf2,
                                       3);

    printf("======= Finished Euclidean Tests =======\n");
}
