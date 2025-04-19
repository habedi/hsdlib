#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

extern void run_utils_tests(void);
extern void run_sqeuclidean_dist_tests(void);
extern void run_cosine_sim_tests(void);
extern void run_dot_sim_tests(void);
extern void run_manhattan_dist_tests(void);
extern void run_hamming_dist_tests(void);
extern void run_jaccard_sim_tests(void);

int main(void) {
    printf("Starting Hsdlib Test Suite...\n");

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
