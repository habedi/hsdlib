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
    hsd_set_manual_backend(HSD_BACKEND_AUTO);
    const char *backend_auto = hsd_get_backend();
    printf("INFO: Auto backend string: \"%s\"\n", backend_auto ? backend_auto : "NULL");
    if (backend_auto != NULL) {
        printf("PASS: hsd_get_backend() returned non-NULL in AUTO mode.\n");
    } else {
        fprintf(stderr, "FAIL: hsd_get_backend() returned NULL in AUTO mode.\n");
        g_test_failed++;
    }

    hsd_status_t status = hsd_set_manual_backend(HSD_BACKEND_SCALAR);
    if (status == HSD_SUCCESS) {
        const char *backend_manual = hsd_get_backend();
        printf("INFO: Forced SCALAR backend string: \"%s\"\n",
               backend_manual ? backend_manual : "NULL");
        const char *expected_manual = "Forced Scalar";
        if (backend_manual != NULL && strcmp(backend_manual, expected_manual) == 0) {
            printf("PASS: hsd_get_backend() returned expected string for forced SCALAR.\n");
        } else {
            fprintf(stderr, "FAIL: hsd_get_backend() check failed for forced SCALAR.\n");
            fprintf(stderr, "      Expected: \"%s\"\n", expected_manual);
            fprintf(stderr, "      Actual:   \"%s\"\n", backend_manual ? backend_manual : "NULL");
            g_test_failed++;
        }
    } else {
        fprintf(stderr, "FAIL: Could not set manual backend to SCALAR (status %d).\n", status);
        g_test_failed++;
    }
    hsd_set_manual_backend(HSD_BACKEND_AUTO);
    printf("\n");

    printf("-- Running test: hsd_has_avx512 check --\n");
    int has_avx512 = hsd_has_avx512();
    printf("INFO: hsd_has_avx512() returned: %d\n", has_avx512);
#if defined(__x86_64__) || defined(_M_X64)
    bool expected_avx512_runtime = hsd_cpu_has_avx512f();
    if (has_avx512 == (int)expected_avx512_runtime) {
        printf("PASS: hsd_has_avx512() returned %d consistent with runtime check.\n", has_avx512);
    } else {
        fprintf(stderr, "FAIL: hsd_has_avx512() returned %d but runtime check indicates %d.\n",
                has_avx512, (int)expected_avx512_runtime);
        g_test_failed++;
    }
#else
    if (has_avx512 == 0) {
        printf("PASS: hsd_has_avx512() returned 0 as expected on non-x86 platform.\n");
    } else {
        fprintf(stderr, "FAIL: hsd_has_avx512() returned %d but expected 0 on non-x86 platform.\n",
                has_avx512);
        g_test_failed++;
    }
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

    if (fp_status.ftz_enabled == false && fp_status.daz_enabled == false) {
        printf("INFO: FTZ/DAZ status check not supported or failed on this platform.\n");
    } else {
        printf(
            "PASS: hsd_get_fp_mode_status executed successfully (values depend on runtime "
            "state).\n");
    }
    printf("\n");

    printf("======= Finished Utilities Tests =======\n");
}
