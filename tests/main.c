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
    if (strcasecmp(str, "AVX512DQ") == 0) return HSD_BACKEND_AVX512DQ;
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

static void run_backend_parser_tests(void) {
    const struct {
        const char* name;
        HSD_Backend expected;
    } cases[] = {
        {NULL, HSD_BACKEND_AUTO},           {"AUTO", HSD_BACKEND_AUTO},
        {"SCALAR", HSD_BACKEND_SCALAR},     {"AVX", HSD_BACKEND_AVX},
        {"AVX2", HSD_BACKEND_AVX2},         {"AVX512F", HSD_BACKEND_AVX512F},
        {"AVX512BW", HSD_BACKEND_AVX512BW}, {"AVX512DQ", HSD_BACKEND_AVX512DQ},
        {"avx512dq", HSD_BACKEND_AVX512DQ}, {"AVX512VPOPCNTDQ", HSD_BACKEND_AVX512VPOPCNTDQ},
        {"NEON", HSD_BACKEND_NEON},         {"SVE", HSD_BACKEND_SVE},
        {"unknown", HSD_BACKEND_AUTO},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (backend_from_string(cases[i].name) != cases[i].expected) {
            fprintf(stderr, "FAIL: Backend parsing for %s\n",
                    cases[i].name ? cases[i].name : "NULL");
            g_test_failed++;
        }
    }
}

int main(void) {
    g_test_failed = 0;
    run_backend_parser_tests();
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
