#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

void run_jaccard_sim_tests(void) {
    printf("\n======= Running Jaccard Similarity Tests (uint16_t) =======\n");

    hsd_func_u16_f32 func_ptr = hsd_sim_jaccard_u16;
    const char *func_name = "hsd_sim_jaccard_u16";

    const uint16_t vec1[] = {1, 2, 0};
    const uint16_t vec2[] = {1, 0, 3};
    // dot=1, nAsq=1+4=5, nBsq=1+9=10
    // sim = 1 / (5 + 10 - 1) = 1 / 14
    run_test_f32_u16_input(func_ptr, func_name, "Basic Test 1", vec1, vec2, 3, 1.0f / 14.0f, 1e-7f);

    const uint16_t v_ident1[] = {10, 20, 30, 40};
    const uint16_t v_ident2[] = {10, 20, 30, 40};
    run_test_f32_u16_input(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 4, 1.0f,
                           1e-7f);

    const uint16_t v_ortho1[] = {1, 0, 5, 0};
    const uint16_t v_ortho2[] = {0, 1, 0, 9};
    // dot=0, nAsq=1+25=26, nBsq=1+81=82
    // sim = 0 / (26 + 82 - 0) = 0
    run_test_f32_u16_input(func_ptr, func_name, "Orthogonal Vectors", v_ortho1, v_ortho2, 4, 0.0f,
                           1e-7f);

    const uint16_t v_zero1[] = {0, 0, 0};
    const uint16_t v_non_zero1[] = {3, 4, 0};
    run_test_f32_u16_input(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero1, v_non_zero1, 3,
                           0.0f, 1e-7f);
    run_test_f32_u16_input(func_ptr, func_name, "Non-Zero vs Zero Vector", v_non_zero1, v_zero1, 3,
                           0.0f, 1e-7f);

    run_test_f32_u16_input(func_ptr, func_name, "Zero Vector vs Zero Vector", v_zero1, v_zero1, 3,
                           1.0f, 1e-7f);

    const uint16_t v_dummy[1] = {0};
    run_test_f32_u16_input(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 1.0f, 1e-7f);

    const uint16_t v1_a[] = {5};
    const uint16_t v1_b[] = {10};
    // dot=50, nAsq=25, nBsq=100
    // sim = 50 / (25 + 100 - 50) = 50 / 75 = 2/3
    run_test_f32_u16_input(func_ptr, func_name, "One Dimension", v1_a, v1_b, 1, 2.0f / 3.0f, 1e-7f);

    const uint16_t v3_a[] = {1, 2, 3};
    const uint16_t v3_b[] = {4, 5, 6};
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3,
                           simple_jaccard_sim_u16(v3_a, v3_b, 3), 1e-6f);

    uint16_t v7_a[7], v7_b[7];
    for (int i = 0; i < 7; ++i) {
        v7_a[i] = (uint16_t)(i + 1);
        v7_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7,
                           simple_jaccard_sim_u16(v7_a, v7_b, 7), 1e-6f);

    uint16_t v16_a[16], v16_b[16];
    for (int i = 0; i < 16; ++i) {
        v16_a[i] = (uint16_t)(i + 1);
        v16_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16,
                           simple_jaccard_sim_u16(v16_a, v16_b, 16), 1e-6f);

    uint16_t v33_a[33], v33_b[33];
    for (int i = 0; i < 33; ++i) {
        v33_a[i] = (uint16_t)(i + 1);
        v33_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 33", v33_a, v33_b, 33,
                           simple_jaccard_sim_u16(v33_a, v33_b, 33), 1e-5f);

    const uint16_t v_max1[] = {65535, 0};
    const uint16_t v_max2[] = {65535, 65535};
    run_test_f32_u16_input(func_ptr, func_name, "Max Value Test", v_max1, v_max2, 2,
                           simple_jaccard_sim_u16(v_max1, v_max2, 2), 1e-6f);

    const uint16_t v_ok[] = {1, 2, 3};

    run_test_expect_failure_status_u16(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_u16(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_sim_jaccard_u16(v_ok, v_ok, 3, NULL);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer: result [%s] (Correctly returned status %d)\n", func_name,
               status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer: result [%s]\n", func_name);
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    // --- Large Vector Tests ---
    printf("-- Running Large Vector Tests [%s] --\n", func_name);
    const size_t LARGE_N1 = 4096;
    const size_t LARGE_N2 = 4096 + 7;  // Test remainder handling

    // Allocate memory
    uint16_t *large_a1 = (uint16_t *)malloc(LARGE_N1 * sizeof(uint16_t));
    uint16_t *large_b1 = (uint16_t *)malloc(LARGE_N1 * sizeof(uint16_t));
    uint16_t *large_a2 = (uint16_t *)malloc(LARGE_N2 * sizeof(uint16_t));
    uint16_t *large_b2 = (uint16_t *)malloc(LARGE_N2 * sizeof(uint16_t));

    if (!large_a1 || !large_b1 || !large_a2 || !large_b2) {
        fprintf(stderr, "FAIL: Failed to allocate memory for large vector tests [%s]\n", func_name);
        g_test_failed++;
        free(large_a1);
        free(large_b1);
        free(large_a2);
        free(large_b2);
    } else {
        // Initialize vectors (example patterns)
        for (size_t i = 0; i < LARGE_N1; ++i) {
            large_a1[i] = (uint16_t)(i * 13);
            large_b1[i] = (uint16_t)((i + 5) * 17);
        }
        for (size_t i = 0; i < LARGE_N2; ++i) {
            large_a2[i] = (uint16_t)(i * 11);
            large_b2[i] = (uint16_t)((i + 3) * 19);
        }

        // Run tests
        run_test_f32_u16_input(func_ptr, func_name, "Large Dimension (N=4096)", large_a1, large_b1,
                               LARGE_N1, simple_jaccard_sim_u16(large_a1, large_b1, LARGE_N1),
                               1e-3f);

        run_test_f32_u16_input(func_ptr, func_name, "Large Dimension (N=4096+7)", large_a2,
                               large_b2, LARGE_N2,
                               simple_jaccard_sim_u16(large_a2, large_b2, LARGE_N2), 1e-3f);

        // Free memory
        free(large_a1);
        free(large_b1);
        free(large_a2);
        free(large_b2);
    }
    printf("-- Finished Large Vector Tests [%s] --\n", func_name);
    // --- End Large Vector Tests ---

    printf("======= Finished Jaccard Similarity Tests (uint16_t) =======\n");
}
