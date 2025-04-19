#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"

extern void run_hsdlib_header_tests(void);
extern void run_euclidean_tests(void);
extern void run_cosine_tests(void);
extern void run_dot_tests(void);
extern void run_manhattan_tests(void);
extern void run_hamming_tests(void);
extern void run_jaccard_tests(void);

int main(void) {
    printf("Starting Hsdlib Test Suite...\n");

    g_test_failed = 0;

    run_hsdlib_header_tests();
    run_euclidean_tests();
    run_cosine_tests();
    run_dot_tests();
    run_manhattan_tests();
    run_hamming_tests();
    run_jaccard_tests();

    printf("\n--- Test Suite Summary ---\n");
    if (g_test_failed > 0) {
        printf("Overall Result: FAIL (%d test case(s) failed)\n", g_test_failed);
        return EXIT_FAILURE;
    } else {
        printf("Overall Result: PASS\n");
        return EXIT_SUCCESS;
    }
}
