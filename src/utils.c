#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if !defined(__EMSCRIPTEN__)
#include <xmmintrin.h>
#endif
#endif

const char *hsd_get_backend(void) {
#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)
    return "AVX512 (VPOPCNTDQ)";
#elif defined(__AVX512BW__) && defined(__AVX512F__)
    return "AVX512BW";
#elif defined(__AVX512F__)
    return "AVX512F";
#elif defined(__AVX2__)
    return "AVX2";
#elif defined(__AVX__)
    return "AVX";
#elif defined(__ARM_FEATURE_SVE)
    return "SVE";
#elif defined(__ARM_NEON)
    return "NEON";
#else
    return "Scalar";
#endif
}

int hsd_has_avx512(void) {
#if defined(__AVX512F__)
    return 1;
#else
    return 0;
#endif
}

hsd_fp_status_t hsd_get_fp_mode_status(void) {
    hsd_fp_status_t status = {-1, -1};
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if !defined(__EMSCRIPTEN__)
#define HSD_MXCSR_FTZ_BIT (1 << 15)
#define HSD_MXCSR_DAZ_BIT (1 << 6)
    unsigned int mxcsr = _mm_getcsr();
    status.ftz_enabled = (mxcsr & HSD_MXCSR_FTZ_BIT) ? 1 : 0;
    status.daz_enabled = (mxcsr & HSD_MXCSR_DAZ_BIT) ? 1 : 0;
#undef HSD_MXCSR_FTZ_BIT
#undef HSD_MXCSR_DAZ_BIT
#endif
#elif defined(__aarch64__)
#define HSD_FPCR_FZ_BIT (1UL << 24)
    unsigned long fpcr_val;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr_val));
    status.ftz_enabled = (fpcr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;
#undef HSD_FPCR_FZ_BIT
#elif defined(__arm__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
#define HSD_FPCR_FZ_BIT (1UL << 24)
    unsigned int fpscr_val;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr_val));
    status.ftz_enabled = (fpscr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;
#undef HSD_FPCR_FZ_BIT
#else
    hsd_log("Warning: Could not determine FTZ/DAZ status for this platform.");
#endif
    return status;
}
