#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

void run_dot_sim_tests(void) {
    printf("\n======= Running Dot Product Similarity Tests =======\n");

    hsd_func_f32_f32 func_ptr = hsd_sim_dot_f32;
    const char *func_name = "hsd_sim_dot_f32";

    const float vec1[] = {1.0f, 2.0f, 3.0f};
    const float vec2[] = {4.0f, 5.0f, 6.0f};

    run_test_f32(func_ptr, func_name, "Basic Test 1", vec1, vec2, 3, 32.0f, 1e-6f);

    const float v_ident1[] = {1.1f, -2.2f, 3.3f, -4.4f};
    const float v_ident2[] = {1.1f, -2.2f, 3.3f, -4.4f};
    run_test_f32(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 4,
                 simple_dot_f32(v_ident1, v_ident2, 4), 1e-5f);  // Use helper for precision

    const float v_para1[] = {1.0f, 2.0f, 3.0f};
    const float v_para2[] = {2.0f, 4.0f, 6.0f};
    run_test_f32(func_ptr, func_name, "Parallel Vectors", v_para1, v_para2, 3, 28.0f, 1e-7f);

    const float v_anti1[] = {1.0f, 2.0f, 3.0f};
    const float v_anti2[] = {-1.0f, -2.0f, -3.0f};
    run_test_f32(func_ptr, func_name, "Anti-parallel Vectors", v_anti1, v_anti2, 3, -14.0f, 1e-7f);

    const float v_anti3[] = {1.0f, 2.0f, 3.0f};
    const float v_anti4[] = {-2.0f, -4.0f, -6.0f};
    run_test_f32(func_ptr, func_name, "Anti-parallel Vectors (Scaled)", v_anti3, v_anti4, 3, -28.0f,
                 1e-7f);

    const float v_ortho1[] = {1.0f, 0.0f, 0.0f};
    const float v_ortho2[] = {0.0f, 1.0f, 0.0f};
    run_test_f32(func_ptr, func_name, "Orthogonal Vectors", v_ortho1, v_ortho2, 3, 0.0f, 1e-7f);

    const float v_ortho3[] = {1.0f, 1.0f};
    const float v_ortho4[] = {-1.0f, 1.0f};
    run_test_f32(func_ptr, func_name, "Orthogonal Vectors 2D", v_ortho3, v_ortho4, 2, 0.0f, 1e-7f);

    const float v_zero1[] = {0.0f, 0.0f, 0.0f};
    const float v_non_zero1[] = {3.0f, 4.0f, 0.0f};
    run_test_f32(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero1, v_non_zero1, 3, 0.0f,
                 1e-7f);
    run_test_f32(func_ptr, func_name, "Non-Zero vs Zero Vector", v_non_zero1, v_zero1, 3, 0.0f,
                 1e-7f);
    run_test_f32(func_ptr, func_name, "Zero Vector vs Zero Vector", v_zero1, v_zero1, 3, 0.0f,
                 1e-7f);

    const float v_neg1[] = {-1.0f, -2.0f};
    const float v_neg2[] = {-4.0f, -5.0f};
    run_test_f32(func_ptr, func_name, "Negative Coordinates", v_neg1, v_neg2, 2, 14.0f, 1e-7f);

    const float v_dummy[1] = {0.0f};
    run_test_f32(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 0.0f, 1e-7f);

    const float v1_a[] = {5.5f};
    const float v1_b[] = {-2.0f};
    run_test_f32(func_ptr, func_name, "One Dimension Negative", v1_a, v1_b, 1, -11.0f, 1e-7f);
    const float v1_c[] = {5.5f};
    const float v1_d[] = {2.0f};
    run_test_f32(func_ptr, func_name, "One Dimension Positive", v1_c, v1_d, 1, 11.0f, 1e-7f);

    const float v3_a[] = {1, 2, 3};
    const float v3_b[] = {4, 5, 6};
    run_test_f32(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3, simple_dot_f32(v3_a, v3_b, 3),
                 1e-6f);

    float v7_a[7], v7_b[7];
    for (int i = 0; i < 7; ++i) {
        v7_a[i] = (float)i + 1.0f;
        v7_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7, simple_dot_f32(v7_a, v7_b, 7),
                 1e-6f);

    float v8_a[8], v8_b[8];
    for (int i = 0; i < 8; ++i) {
        v8_a[i] = (float)i + 1.0f;
        v8_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 8", v8_a, v8_b, 8, simple_dot_f32(v8_a, v8_b, 8),
                 1e-5f);

    float v9_a[9], v9_b[9];
    for (int i = 0; i < 9; ++i) {
        v9_a[i] = (float)i + 1.0f;
        v9_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 9", v9_a, v9_b, 9, simple_dot_f32(v9_a, v9_b, 9),
                 1e-5f);

    float v15_a[15], v15_b[15];
    for (int i = 0; i < 15; ++i) {
        v15_a[i] = (float)i + 1.0f;
        v15_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 15", v15_a, v15_b, 15,
                 simple_dot_f32(v15_a, v15_b, 15), 1e-5f);

    float v16_a[16], v16_b[16];
    for (int i = 0; i < 16; ++i) {
        v16_a[i] = (float)i + 1.0f;
        v16_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16,
                 simple_dot_f32(v16_a, v16_b, 16), 1e-5f);

    float v17_a[17], v17_b[17];
    for (int i = 0; i < 17; ++i) {
        v17_a[i] = (float)i + 1.0f;
        v17_b[i] = (float)i + 2.0f;
    }
    run_test_f32(func_ptr, func_name, "Dimension 17", v17_a, v17_b, 17,
                 simple_dot_f32(v17_a, v17_b, 17), 1e-5f);

    const float v_small1[] = {1e-20f, 2e-20f};
    const float v_small2[] = {3e-20f, 4e-20f};
    run_test_f32(func_ptr, func_name, "Small Values", v_small1, v_small2, 2,
                 simple_dot_f32(v_small1, v_small2, 2), 1e-45f);

    const float v_small3[] = {1e-30f, 0.0f};
    const float v_small4[] = {0.0f, 1e-30f};
    run_test_f32(func_ptr, func_name, "Small Values (Orthogonal)", v_small3, v_small4, 2, 0.0f,
                 1e-35f);

    const float v_large1[] = {1e19f, 1e19f};
    const float v_large2[] = {2e19f, 2e19f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "Large Values (Expect Failure)",
                                       v_large1, v_large2, 2);

    float v_overflow1[4];
    float v_overflow2[4];
    for (int i = 0; i < 4; ++i) {
        v_overflow1[i] = 1.5e+19f;
        v_overflow2[i] = 1.5e+19f;
    }
    run_test_expect_failure_status_f32(func_ptr, func_name, "Potential Overflow Sum", v_overflow1,
                                       v_overflow2, 4);

    const float v_ok[] = {1.0f, 2.0f, 3.0f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_sim_dot_f32(v_ok, v_ok, 3, NULL);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer: result [%s] (Correctly returned status %d)\n", func_name,
               status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer: result [%s]\n", func_name);
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    const float v_nan1[] = {1.0f, NAN, 3.0f};
    const float v_nan2[] = {1.0f, 2.0f, NAN};
    run_test_expect_failure_status_f32(func_ptr, func_name, "NaN Input Vec A", v_nan1, v_ok, 3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "NaN Input Vec B", v_ok, v_nan2, 3);

    const float v_inf1[] = {1.0f, INFINITY, 3.0f};
    const float v_inf2[] = {-INFINITY, 2.0f, 3.0f};
    run_test_expect_failure_status_f32(func_ptr, func_name, "Infinity Input Vec A", v_inf1, v_ok,
                                       3);
    run_test_expect_failure_status_f32(func_ptr, func_name, "Infinity Input Vec B", v_ok, v_inf2,
                                       3);

    // --- Large Vector Tests ---
    printf("-- Running Large Vector Tests [%s] --\n", func_name);
    const size_t LARGE_N1 = 4096;
    const size_t LARGE_N2 = 4096 + 7;  // Test remainder handling

    // Allocate memory
    float *large_a1 = (float *)malloc(LARGE_N1 * sizeof(float));
    float *large_b1 = (float *)malloc(LARGE_N1 * sizeof(float));
    float *large_a2 = (float *)malloc(LARGE_N2 * sizeof(float));
    float *large_b2 = (float *)malloc(LARGE_N2 * sizeof(float));

    if (!large_a1 || !large_b1 || !large_a2 || !large_b2) {
        fprintf(stderr, "FAIL: Failed to allocate memory for large vector tests [%s]\n", func_name);
        g_test_failed++;
        free(large_a1);
        free(large_b1);
        free(large_a2);
        free(large_b2);
    } else {
        // Initialize vectors
        for (size_t i = 0; i < LARGE_N1; ++i) {
            large_a1[i] = (float)(i % 13) * 0.5f - 3.0f;
            large_b1[i] = (float)((i + 5) % 17) * 0.3f + 1.0f;
        }
        for (size_t i = 0; i < LARGE_N2; ++i) {
            large_a2[i] = (float)(i % 11) * 0.4f - 2.0f;
            large_b2[i] = (float)((i + 3) % 19) * 0.6f - 0.5f;
        }

        // Run tests (using slightly larger tolerance due to potential FP accumulation)
        run_test_f32(func_ptr, func_name, "Large Dimension (N=4096)", large_a1, large_b1, LARGE_N1,
                     simple_dot_f32(large_a1, large_b1, LARGE_N1), 1e-3f);

        run_test_f32(func_ptr, func_name, "Large Dimension (N=4096+7)", large_a2, large_b2,
                     LARGE_N2, simple_dot_f32(large_a2, large_b2, LARGE_N2), 1e-3f);

        // Free memory
        free(large_a1);
        free(large_b1);
        free(large_a2);
        free(large_b2);
    }
    printf("-- Finished Large Vector Tests [%s] --\n", func_name);
    // --- End Large Vector Tests ---

    printf("======= Finished Dot Product Similarity Tests =======\n");
}
