#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

void run_hamming_dist_tests(void) {
    printf("\n======= Running Hamming Distance Tests (uint8_t) =======\n");

    hsd_func_u8_u64 func_ptr = hsd_dist_hamming_u8;
    const char *func_name = "hsd_dist_hamming_u8";

    // XOR = {0b001, 0b011, 0b010, 0b000} = {1, 3, 2, 0}
    // Popcounts = {1, 2, 1, 0} -> Sum = 4
    const uint8_t vec1[] = {0b101, 0b101, 0b110, 0b111};
    const uint8_t vec2[] = {0b100, 0b110, 0b100, 0b111};
    run_test_u64_u8_input(func_ptr, func_name, "Basic Test 1", vec1, vec2, 4, 4);

    const uint8_t v_ident1[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    const uint8_t v_ident2[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    run_test_u64_u8_input(func_ptr, func_name, "Identical Vectors", v_ident1, v_ident2, 8, 0);

    const uint8_t v_diff1[] = {0x00, 0x00, 0x00, 0x00};
    const uint8_t v_diff2[] = {0xFF, 0xFF, 0xFF, 0xFF};
    run_test_u64_u8_input(func_ptr, func_name, "Completely Different", v_diff1, v_diff2, 4, 32);

    const uint8_t v_zero[] = {0, 0, 0, 0, 0};
    const uint8_t v_non_zero[] = {0b1, 0b0, 0b10, 0b0, 0b100};  // popcounts {1, 0, 1, 0, 1}
    run_test_u64_u8_input(func_ptr, func_name, "Zero Vector vs Non-Zero", v_zero, v_non_zero, 5, 3);
    run_test_u64_u8_input(func_ptr, func_name, "Non-Zero vs Zero Vector", v_non_zero, v_zero, 5, 3);
    run_test_u64_u8_input(func_ptr, func_name, "Zero Vector vs Zero Vector", v_zero, v_zero, 5, 0);

    const uint8_t v_dummy[1] = {0};
    run_test_u64_u8_input(func_ptr, func_name, "Zero Dimension", v_dummy, v_dummy, 0, 0);

    const uint8_t v1_a[] = {0xFF};  // 8 bits
    const uint8_t v1_b[] = {0x00};  // 0 bits
    run_test_u64_u8_input(func_ptr, func_name, "One Dimension", v1_a, v1_b, 1, 8);

    // XOR = {0b01, 0b11, 0b10} = {1, 3, 2} -> Popcounts {1, 2, 1} -> Sum = 4
    const uint8_t v3_a[] = {0b10, 0b01, 0b11};
    const uint8_t v3_b[] = {0b11, 0b10, 0b01};
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 3", v3_a, v3_b, 3, 4);

    uint8_t v7_a[7], v7_b[7];
    uint8_t v8_a[8], v8_b[8];
    uint8_t v9_a[9], v9_b[9];
    uint8_t v15_a[15], v15_b[15];
    uint8_t v16_a[16], v16_b[16];
    uint8_t v17_a[17], v17_b[17];
    uint8_t v31_a[31], v31_b[31];
    uint8_t v32_a[32], v32_b[32];
    uint8_t v33_a[33], v33_b[33];
    uint8_t v63_a[63], v63_b[63];
    uint8_t v64_a[64], v64_b[64];
    uint8_t v65_a[65], v65_b[65];

    // Fill with alternating patterns that XOR to 0xFF (8 bits)
#define INIT_HAMMING_VEC(vec_a, vec_b, size) \
    for (int i = 0; i < size; ++i) {         \
        vec_a[i] = 0xAA;                     \
        vec_b[i] = 0x55;                     \
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

    run_test_u64_u8_input(func_ptr, func_name, "Dimension 7", v7_a, v7_b, 7, 7 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 8", v8_a, v8_b, 8, 8 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 9", v9_a, v9_b, 9, 9 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 15", v15_a, v15_b, 15, 15 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 16", v16_a, v16_b, 16, 16 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 17", v17_a, v17_b, 17, 17 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 31", v31_a, v31_b, 31, 31 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 32", v32_a, v32_b, 32, 32 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 33", v33_a, v33_b, 33, 33 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 63", v63_a, v63_b, 63, 63 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 64", v64_a, v64_b, 64, 64 * 8);
    run_test_u64_u8_input(func_ptr, func_name, "Dimension 65", v65_a, v65_b, 65, 65 * 8);

    const uint8_t v_ok[] = {1, 0, 1};
    run_test_expect_failure_status_u8(func_ptr, func_name, "NULL Pointer: vec a", NULL, v_ok, 3);
    run_test_expect_failure_status_u8(func_ptr, func_name, "NULL Pointer: vec b", v_ok, NULL, 3);

    printf("-- Running test: NULL Pointer: result [%s] (n=3) --\n", func_name);
    hsd_log("Test setup: Expecting HSD_ERR_NULL_PTR status");
    hsd_status_t status = hsd_dist_hamming_u8(v_ok, v_ok, 3, NULL);
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
    uint8_t *large_a1 = (uint8_t *)malloc(LARGE_N1 * sizeof(uint8_t));
    uint8_t *large_b1 = (uint8_t *)malloc(LARGE_N1 * sizeof(uint8_t));
    uint8_t *large_a2 = (uint8_t *)malloc(LARGE_N2 * sizeof(uint8_t));
    uint8_t *large_b2 = (uint8_t *)malloc(LARGE_N2 * sizeof(uint8_t));

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
            large_a1[i] = (uint8_t)(i ^ (i >> 8));
            large_b1[i] = (uint8_t)((i + 33) ^ (i >> 6));
        }
        for (size_t i = 0; i < LARGE_N2; ++i) {
            large_a2[i] = (uint8_t)(i * 3);
            large_b2[i] = (uint8_t)(~(i * 5));
        }

        // Run tests
        run_test_u64_u8_input(func_ptr, func_name, "Large Dimension (N=4096)", large_a1, large_b1,
                              LARGE_N1, simple_hamming_u8(large_a1, large_b1, LARGE_N1));

        run_test_u64_u8_input(func_ptr, func_name, "Large Dimension (N=4096+7)", large_a2, large_b2,
                              LARGE_N2, simple_hamming_u8(large_a2, large_b2, LARGE_N2));

        // Free memory
        free(large_a1);
        free(large_b1);
        free(large_a2);
        free(large_b2);
    }
    printf("-- Finished Large Vector Tests [%s] --\n", func_name);
    // --- End Large Vector Tests ---

    printf("======= Finished Hamming Distance Tests (uint8_t) =======\n");
}
