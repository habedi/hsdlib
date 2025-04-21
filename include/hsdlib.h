#ifndef HSDLIB_H
#define HSDLIB_H

#ifdef HSDLIB_NO_CHECKS
#define HSD_ALLOW_FP_CHECKS 0
#else
#define HSD_ALLOW_FP_CHECKS 1
#endif

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define HSD_ASM __asm__ volatile
#else
#define HSD_ASM asm volatile
#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

typedef enum {
    HSD_SUCCESS = 0,
    HSD_ERR_NULL_PTR = -1,
    HSD_ERR_INVALID_INPUT = -3,
    HSD_ERR_CPU_NOT_SUPPORTED = -4,
    HSD_FAILURE = -99
} HSD_Status;

typedef HSD_Status hsd_status_t;

typedef struct {
    int ftz_enabled;
    int daz_enabled;
} hsd_fp_status_t;

typedef enum {
    HSD_BACKEND_AUTO = 0,
    HSD_BACKEND_SCALAR,
    HSD_BACKEND_AVX,
    HSD_BACKEND_AVX2,
    HSD_BACKEND_AVX512F,
    HSD_BACKEND_AVX512BW,
    HSD_BACKEND_AVX512DQ,
    HSD_BACKEND_AVX512VPOPCNTDQ,
    HSD_BACKEND_NEON,
    HSD_BACKEND_SVE
} HSD_Backend;

#ifdef __cplusplus
extern "C" {
#endif

hsd_status_t hsd_dist_sqeuclidean_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_dist_manhattan_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_dist_hamming_u8(const uint8_t *a, const uint8_t *b, size_t n, uint64_t *result);

hsd_status_t hsd_sim_dot_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_sim_cosine_f32(const float *a, const float *b, size_t n, float *result);
hsd_status_t hsd_sim_jaccard_u16(const uint16_t *a, const uint16_t *b, size_t n, float *result);

const char *hsd_get_backend(void);
bool hsd_has_avx512(void);
hsd_fp_status_t hsd_get_fp_mode_status(void);

hsd_status_t hsd_set_manual_backend(HSD_Backend backend);
HSD_Backend hsd_get_current_backend_choice(void);

#if defined(__x86_64__) || defined(_M_X64)
bool hsd_cpu_has_avx(void);
bool hsd_cpu_has_avx2(void);
bool hsd_cpu_has_fma(void);
bool hsd_cpu_has_avx512f(void);
bool hsd_cpu_has_avx512bw(void);
bool hsd_cpu_has_avx512dq(void);
bool hsd_cpu_has_avx512vpopcntdq(void);
#elif defined(__aarch64__)
bool hsd_cpu_has_neon(void);
bool hsd_cpu_has_sve(void);
#endif

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

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>

static inline float hsd_internal_hsum_avx_f32(__m256 acc) {
    __m128 hsum_128 = _mm_add_ps(_mm256_castps256_ps128(acc), _mm256_extractf128_ps(acc, 1));
    hsum_128 = _mm_hadd_ps(hsum_128, hsum_128);
    hsum_128 = _mm_hadd_ps(hsum_128, hsum_128);
    return _mm_cvtss_f32(hsum_128);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
