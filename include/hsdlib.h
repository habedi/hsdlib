#ifndef HSDLIB_H
#define HSDLIB_H

#include <stddef.h>
#include <stdint.h>

typedef int hsd_status_t;

enum {
    HSD_SUCCESS       =  0,
    HSD_ERR_NULL_PTR  = -1,
    HSD_ERR_DIM_MISMATCH = -2,
    HSD_ERR_UNSUPPORTED  = -3,
    HSD_FAILURE       = -99
};

#ifdef NO_AVX512
#undef __AVX512F__
#undef __AVX512BW__
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Distance metrics
hsd_status_t hsd_euclidean_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_cosine_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_manhattan_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_hamming_i8(const int8_t *a, const int8_t *b, size_t n, float *result);

// Similarity measures
hsd_status_t hsd_dot_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_jaccard_u16(const uint16_t *a, const uint16_t *b, size_t n, float *result);

// Vector operations
hsd_status_t hsd_normalize_l2_f32(float *vec, size_t n);

// Utility functions
static const char *hsd_get_backend(void) {
#if defined(__AVX512BW__) && defined(__AVX512F__)
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

static int hsd_has_avx512(void) {
#if defined(__AVX512F__)
    return 1;
#else
    return 0;
#endif
}

#ifdef HSD_DEBUG
#include <stdarg.h>
#include <stdio.h>

static inline void hsdlib_internal_do_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[HSDLIB_DEBUG] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(stderr);
}

#define hsd_log(...) hsdlib_internal_do_log(__VA_ARGS__)

#else
#define hsd_log(...) ((void)0)
#endif

#if defined(__AVX__)

#include <immintrin.h>

static float hsd_internal_hsum_avx_f32(__m256 acc) {
    __m128 upper = _mm256_extractf128_ps(acc, 1);
    __m128 lower = _mm256_castps256_ps128(acc);
    __m128 sum128 = _mm_add_ps(lower, upper);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    return _mm_cvtss_f32(sum128);
}
#endif

typedef struct {
    int ftz_enabled;
    int daz_enabled;
} hsd_fp_status_t;

static hsd_fp_status_t hsd_get_fp_mode_status(void) {
    hsd_fp_status_t status = {-1, -1};

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if !defined(__EMSCRIPTEN__)
#include <xmmintrin.h>

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
    // FZ bit typically controls both FTZ and DAZ on AArch64
    status.ftz_enabled = (fpcr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;  // Assume DAZ follows FZ
#undef HSD_FPCR_FZ_BIT

#elif defined(__arm__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
// Check for VFP/NEON hardware float support on ARM32
#define HSD_FPCR_FZ_BIT (1UL << 24)
    unsigned int fpscr_val;
    // Use VMRS to read FPSCR (Floating Point Status and Control Register)
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr_val));
    // FZ bit typically controls both FTZ and DAZ here too
    status.ftz_enabled = (fpscr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;  // Assume DAZ follows FZ
#undef HSD_FPCR_FZ_BIT

#else
    // Platform isn't recognized or lacks hardware float / control registers
    // status remains {-1, -1}
    hsd_log("Warning: Could not determine FTZ/DAZ status for this platform.");
#endif

    return status;
}

#ifdef __cplusplus
}
#endif

#endif
