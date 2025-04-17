#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

void run_jaccard_tests(void) {
    printf("\n======= Running Jaccard Tests (Tanimoto uint16_t) =======\n");

    hsd_func_u16_f32 func_ptr = hsd_jaccard_u16;
    const char *func_name = "hsd_jaccard_u16";

    const uint16_t vec1[] = {1, 2, 0};
    const uint16_t vec2[] = {1, 0, 3};

    run_test_f32_u16_input(func_ptr, func_name, "Basic Test 1", vec1, vec2, 3, 13.0f / 14.0f,
                           1e-7f);

    const uint16_t v_ident1[] = {10, 20, 30, 40};
    const uint16_t v_ident2[] = {10, 20, 30, 40};
    run_test_f32_u16_input(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 4, 0.0f,
                           1e-7f);

    const uint16_t v_ortho1[] = {1, 0, 5, 0};
    const uint16_t v_ortho2[] = {0, 1, 0, 9};

    run_test_f32_u16_input(func_ptr, func_name, "Orthogonal Vectors", v_ortho1, v_ortho2, 4, 1.0f,
                           1e-7f);

    const uint16_t v_zero1[] = {0, 0, 0};
    const uint16_t v_non_zero1[] = {3, 4, 0};

    run_test_f32_u16_input(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero1, v_non_zero1, 3,
                           1.0f, 1e-7f);
    run_test_f32_u16_input(func_ptr, func_name, "Non-Zero vs Zero Vector", v_non_zero1, v_zero1, 3,
                           1.0f, 1e-7f);

    run_test_f32_u16_input(func_ptr, func_name, "Zero Vector vs Zero Vector", v_zero1, v_zero1, 3,
                           0.0f, 1e-7f);

    const uint16_t v_dummy[1] = {0};
    run_test_f32_u16_input(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 0.0f, 1e-7f);

    const uint16_t v1_a[] = {5};
    const uint16_t v1_b[] = {10};

    run_test_f32_u16_input(func_ptr, func_name, "One Dimension", v1_a, v1_b, 1, 1.0f / 3.0f, 1e-7f);

    const uint16_t v3_a[] = {1, 2, 3};
    const uint16_t v3_b[] = {4, 5, 6};
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3,
                           simple_jaccard_u16(v3_a, v3_b, 3), 1e-6f);

    uint16_t v7_a[7], v7_b[7];
    for (int i = 0; i < 7; ++i) {
        v7_a[i] = (uint16_t)(i + 1);
        v7_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7,
                           simple_jaccard_u16(v7_a, v7_b, 7), 1e-6f);

    uint16_t v16_a[16], v16_b[16];
    for (int i = 0; i < 16; ++i) {
        v16_a[i] = (uint16_t)(i + 1);
        v16_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16,
                           simple_jaccard_u16(v16_a, v16_b, 16), 1e-6f);

    uint16_t v33_a[33], v33_b[33];
    for (int i = 0; i < 33; ++i) {
        v33_a[i] = (uint16_t)(i + 1);
        v33_b[i] = (uint16_t)(i + 2);
    }
    run_test_f32_u16_input(func_ptr, func_name, "Dimension 33", v33_a, v33_b, 33,
                           simple_jaccard_u16(v33_a, v33_b, 33), 1e-5f);

    const uint16_t v_max1[] = {65535, 0};
    const uint16_t v_max2[] = {65535, 65535};
    run_test_f32_u16_input(func_ptr, func_name, "Max Value Test", v_max1, v_max2, 2,
                           simple_jaccard_u16(v_max1, v_max2, 2), 1e-6f);

    const uint16_t v_ok[] = {1, 2, 3};

    run_test_expect_failure_status_u16(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_u16(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_jaccard_u16(v_ok, v_ok, 3, NULL);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer: result [%s] (Correctly returned status %d)\n", func_name,
               status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer: result [%s]\n", func_name);
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    printf("======= Finished Jaccard Tests (Tanimoto uint16_t) =======\n");
}
