// tests/main.c
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "test_common.h"  // Includes hsdlib.h

// Function to convert string to HSD_Backend enum
static HSD_Backend backend_from_string(const char* str) {
    if (!str) return HSD_BACKEND_AUTO;
    if (strcasecmp(str, "SCALAR") == 0) return HSD_BACKEND_SCALAR;
    if (strcasecmp(str, "AVX") == 0) return HSD_BACKEND_AVX;
    if (strcasecmp(str, "AVX2") == 0) return HSD_BACKEND_AVX2;
    if (strcasecmp(str, "AVX512F") == 0) return HSD_BACKEND_AVX512F;
    if (strcasecmp(str, "AVX512BW") == 0) return HSD_BACKEND_AVX512BW;
    if (strcasecmp(str, "AVX512VPOPCNTDQ") == 0) return HSD_BACKEND_AVX512VPOPCNTDQ;
    if (strcasecmp(str, "NEON") == 0) return HSD_BACKEND_NEON;
    if (strcasecmp(str, "SVE") == 0) return HSD_BACKEND_SVE;
    return HSD_BACKEND_AUTO;  // Default
}

extern void run_utils_tests(void);
extern void run_sqeuclidean_dist_tests(void);
extern void run_manhattan_dist_tests(void);
extern void run_hamming_dist_tests(void);
extern void run_cosine_sim_tests(void);
extern void run_dot_sim_tests(void);
extern void run_jaccard_sim_tests(void);

int main(void) {
    const char* forced_backend_str = getenv("HSD_TEST_FORCE_BACKEND");
    HSD_Backend backend_to_force = backend_from_string(forced_backend_str);

    printf("Starting Hsdlib Test Suite...\n");
    if (backend_to_force != HSD_BACKEND_AUTO) {
        printf(">>> Forcing Backend: %s (%d) <<<\n", forced_backend_str, backend_to_force);
        hsd_status_t status = hsd_set_manual_backend(backend_to_force);
        if (status != HSD_SUCCESS) {
            fprintf(stderr, "WARN: Failed to set manual backend %s\n", forced_backend_str);
        }
    } else {
        printf(">>> Using Backend: AUTO <<<\n");
        hsd_set_manual_backend(HSD_BACKEND_AUTO);  // Ensure auto mode
    }

    g_test_failed = 0;

    // Run all tests
    run_manhattan_dist_tests();
    run_sqeuclidean_dist_tests();
    run_hamming_dist_tests();
    run_cosine_sim_tests();
    run_dot_sim_tests();
    run_jaccard_sim_tests();
    run_utils_tests();

    printf("\n--- Test Suite Summary ---\n");
    if (g_test_failed > 0) {
        printf("Overall Result: FAIL (%d test case(s) failed)\n", g_test_failed);
        return EXIT_FAILURE;
    } else {
        printf("Overall Result: PASS\n");
        return EXIT_SUCCESS;
    }
}
