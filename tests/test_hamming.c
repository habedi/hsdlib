#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

void run_hamming_tests(void) {
    printf("\n======= Running Hamming Tests (Binary i8) =======\n");

    hsd_func_i8_f32 func_ptr = hsd_hamming_i8;
    const char *func_name = "hsd_hamming_i8";
    const float TOLERANCE = 1e-5f;

    const int8_t vec1[] = {1, 0, 1, 0, 1, 1, 0, 0};
    const int8_t vec2[] = {1, 1, 0, 0, 1, 0, 1, 0};

    run_test_f32_i8_input(func_ptr, func_name, "Basic Test 1", vec1, vec2, 8, 4.0f, TOLERANCE);

    const int8_t v_ident1[] = {1, 1, 0, 1, 0, 0, 0, 1};
    const int8_t v_ident2[] = {1, 1, 0, 1, 0, 0, 0, 1};
    run_test_f32_i8_input(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 8, 0.0f,
                          TOLERANCE);

    const int8_t v_diff1[] = {0, 0, 0, 0};
    const int8_t v_diff2[] = {1, 1, 1, 1};
    run_test_f32_i8_input(func_ptr, func_name, "Completely Different", v_diff1, v_diff2, 4, 4.0f,
                          TOLERANCE);

    const int8_t v_zero[] = {0, 0, 0, 0, 0};
    const int8_t v_non_zero[] = {1, 0, 1, 0, 1};
    run_test_f32_i8_input(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero, v_non_zero, 5,
                          3.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Non-Zero vs Zero Vector", v_non_zero, v_zero, 5,
                          3.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Zero Vector vs Zero Vector", v_zero, v_zero, 5,
                          0.0f, TOLERANCE);

    const int8_t v_dummy[1] = {0};
    run_test_f32_i8_input(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 0.0f,
                          TOLERANCE);

    const int8_t v1_a[] = {1};
    const int8_t v1_b[] = {0};
    run_test_f32_i8_input(func_ptr, func_name, "One Dimension", v1_a, v1_b, 1, 1.0f, TOLERANCE);

    const int8_t v3_a[] = {1, 0, 1};
    const int8_t v3_b[] = {0, 1, 1};
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3, 2.0f, TOLERANCE);

    int8_t v7_a[7], v7_b[7];
    int8_t v8_a[8], v8_b[8];
    int8_t v9_a[9], v9_b[9];
    int8_t v15_a[15], v15_b[15];
    int8_t v16_a[16], v16_b[16];
    int8_t v17_a[17], v17_b[17];
    int8_t v31_a[31], v31_b[31];
    int8_t v32_a[32], v32_b[32];
    int8_t v33_a[33], v33_b[33];
    int8_t v63_a[63], v63_b[63];
    int8_t v64_a[64], v64_b[64];
    int8_t v65_a[65], v65_b[65];

#define INIT_HAMMING_VEC(vec_a, vec_b, size) \
    for (int i = 0; i < size; ++i) {         \
        vec_a[i] = i % 2;                    \
        vec_b[i] = (i + 1) % 2;              \
    }

    INIT_HAMMING_VEC(v7_a, v7_b, 7);
    INIT_HAMMING_VEC(v8_a, v8_b, 8);
    INIT_HAMMING_VEC(v9_a, v9_b, 9);
    INIT_HAMMING_VEC(v15_a, v15_b, 15);
    INIT_HAMMING_VEC(v16_a, v16_b, 16);
    INIT_HAMMING_VEC(v17_a, v17_b, 17);
    INIT_HAMMING_VEC(v31_a, v31_b, 31);
    INIT_HAMMING_VEC(v32_a, v32_b, 32);
    INIT_HAMMING_VEC(v33_a, v33_b, 33);
    INIT_HAMMING_VEC(v63_a, v63_b, 63);
    INIT_HAMMING_VEC(v64_a, v64_b, 64);
    INIT_HAMMING_VEC(v65_a, v65_b, 65);

#undef INIT_HAMMING_VEC

    run_test_f32_i8_input(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7, 7.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 8", v8_a, v8_b, 8, 8.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 9", v9_a, v9_b, 9, 9.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 15", v15_a, v15_b, 15, 15.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16, 16.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 17", v17_a, v17_b, 17, 17.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 31", v31_a, v31_b, 31, 31.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 32", v32_a, v32_b, 32, 32.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 33", v33_a, v33_b, 33, 33.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 63", v63_a, v63_b, 63, 63.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 64", v64_a, v64_b, 64, 64.0f, TOLERANCE);
    run_test_f32_i8_input(func_ptr, func_name, "Dimension 65", v65_a, v65_b, 65, 65.0f, TOLERANCE);

    const int8_t v_ok[] = {1, 0, 1};
    run_test_expect_failure_status_i8(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_i8(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_hamming_i8(v_ok, v_ok, 3, NULL);
    if (status == HSD_ERR_NULL_PTR) {
        printf("PASS: NULL Pointer: result [%s] (Correctly returned status %d)\n", func_name,
               status);
    } else {
        fprintf(stderr, "FAIL: NULL Pointer: result [%s]\n", func_name);
        fprintf(stderr, "      Expected status %d, but got %d\n", HSD_ERR_NULL_PTR, status);
        g_test_failed++;
    }
    printf("\n");

    printf("======= Finished Hamming Tests (Binary i8) =======\n");
}
