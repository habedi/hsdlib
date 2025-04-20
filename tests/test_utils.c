#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hsdlib.h"
#include "test_common.h"

#if defined(__AVX__)
#include <immintrin.h>
#endif

void run_utils_tests(void) {
    printf("\n======= Running Utilities Tests =======\n");

    printf("-- Running test: hsd_get_backend check --\n");
    const char *backend = hsd_get_backend();
    if (backend != NULL) {
        printf("INFO: hsd_get_backend returned string: \"%s\"\n", backend);

        // Determine the expected backend string based on HSD_TARGET_* first
        const char *expected_backend = NULL;
#if defined(HSD_TARGET_AVX512VPOPCNTDQ)
        expected_backend = "Forced AVX512 (VPOPCNTDQ)";
#elif defined(HSD_TARGET_AVX512BW)
        expected_backend = "Forced AVX512BW";
#elif defined(HSD_TARGET_AVX512)  // Corresponds to HSD_TARGET_AVX512F
        expected_backend = "Forced AVX512F";
#elif defined(HSD_TARGET_AVX2)
        expected_backend = "Forced AVX2";
#elif defined(HSD_TARGET_AVX)
        expected_backend = "Forced AVX";
#elif defined(HSD_TARGET_SVE)
        expected_backend = "Forced SVE";
#elif defined(HSD_TARGET_NEON)
        expected_backend = "Forced NEON";
#elif defined(HSD_TARGET_SCALAR)
        expected_backend = "Forced Scalar";
#else
// No HSD_TARGET_* defined, check auto-detection based on compiler flags
#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)
        expected_backend = "Auto AVX512 (VPOPCNTDQ)";
#elif defined(__AVX512BW__) && defined(__AVX512F__)
        expected_backend = "Auto AVX512BW";
#elif defined(__AVX512F__)
        expected_backend = "Auto AVX512F";
#elif defined(__AVX2__)
        expected_backend = "Auto AVX2";
#elif defined(__AVX__)
        expected_backend = "Auto AVX";
#elif defined(__ARM_FEATURE_SVE)
        expected_backend = "Auto SVE";
#elif defined(__ARM_NEON)
        expected_backend = "Auto NEON";
#else
        expected_backend = "Auto Scalar";
#endif
#endif  // HSD_TARGET_* checks

        // Now compare the actual backend with the expected one
        if (expected_backend != NULL && strcmp(backend, expected_backend) == 0) {
            printf(
                "PASS: hsd_get_backend returned expected string \"%s\" for current build "
                "configuration.\n",
                expected_backend);
        } else {
            fprintf(stderr, "FAIL: hsd_get_backend check failed.\n");
            fprintf(stderr, "      Expected: \"%s\"\n",
                    expected_backend ? expected_backend : "(Could not determine expected)");
            fprintf(stderr, "      Actual:   \"%s\"\n", backend);
            g_test_failed++;
        }

    } else {
        fprintf(stderr, "FAIL: hsd_get_backend returned NULL\n");
        g_test_failed++;
    }
    printf("\n");

    printf("-- Running test: hsd_has_avx512 check --\n");
    int has_avx512 = hsd_has_avx512();
    printf("INFO: hsd_has_avx512() returned: %d\n", has_avx512);
#if defined(__AVX512F__)
    if (has_avx512 == 1) {
        printf("PASS: hsd_has_avx512() returned 1 as expected (__AVX512F__ defined).\n");
    } else {
        fprintf(stderr,
                "FAIL: hsd_has_avx512() returned %d but expected 1 (__AVX512F__ defined).\n",
                has_avx512);
        g_test_failed++;
    }
#else
    if (has_avx512 == 0) {
        printf("PASS: hsd_has_avx512() returned 0 as expected (__AVX512F__ not defined).\n");
    } else {
        fprintf(stderr,
                "FAIL: hsd_has_avx512() returned %d but expected 0 (__AVX512F__ not defined).\n",
                has_avx512);
        g_test_failed++;
    }
#endif
    printf("\n");

    printf("-- Running test: hsd_internal_hsum_avx_f32 check --\n");
#if defined(__AVX__)
    __m256 test_vec = _mm256_set_ps(8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    float expected_sum = 1.0f + 2.0f + 3.0f + 4.0f + 5.0f + 6.0f + 7.0f + 8.0f;
    float actual_sum = hsd_internal_hsum_avx_f32(test_vec);
    float tolerance = FLT_EPSILON * 8.0f;

    if (fabsf(expected_sum - actual_sum) <= tolerance) {
        printf("PASS: hsd_internal_hsum_avx_f32 (Expected: %.8f, Actual: %.8f)\n", expected_sum,
               actual_sum);
    } else {
        fprintf(stderr, "FAIL: hsd_internal_hsum_avx_f32\n");
        fprintf(stderr, "      Expected: %.8f\n", expected_sum);
        fprintf(stderr, "      Actual:   %.8f\n", actual_sum);
        fprintf(stderr, "      Difference: %.8e > Tolerance: %.8e\n",
                fabsf(expected_sum - actual_sum), tolerance);
        g_test_failed++;
    }
#else
    printf("SKIPPED: hsd_internal_hsum_avx_f32 (__AVX__ not defined)\n");
#endif
    printf("\n");

    printf("-- Running test: hsd_get_fp_mode_status check --\n");
    hsd_fp_status_t fp_status = hsd_get_fp_mode_status();
    printf("INFO: hsd_get_fp_mode_status returned:\n");
    printf("      FTZ (Flush-To-Zero): %d ", fp_status.ftz_enabled);
    if (fp_status.ftz_enabled == 1)
        printf("(Enabled)\n");
    else if (fp_status.ftz_enabled == 0)
        printf("(Disabled)\n");
    else
        printf("(Unknown/Unsupported)\n");

    printf("      DAZ (Denormals-Are-Zero): %d ", fp_status.daz_enabled);
    if (fp_status.daz_enabled == 1)
        printf("(Enabled)\n");
    else if (fp_status.daz_enabled == 0)
        printf("(Disabled)\n");
    else
        printf("(Unknown/Unsupported)\n");

    if (fp_status.ftz_enabled == -1 && fp_status.daz_enabled == -1) {
        printf("INFO: FTZ/DAZ status check not supported or failed on this platform.\n");
    } else {
        printf(
            "PASS: hsd_get_fp_mode_status executed successfully (values depend on runtime "
            "state).\n");
    }
    printf("\n");

    printf("======= Finished Utilities Tests =======\n");
}
