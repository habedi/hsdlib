#ifndef HSDLIB_H
#define HSDLIB_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    HSD_SUCCESS = 0,
    HSD_ERR_NULL_PTR = -1,
    HSD_ERR_UNSUPPORTED = -2,
    HSD_ERR_INVALID_INPUT = -3,
    HSD_FAILURE = -99
} HSD_Status;

typedef HSD_Status hsd_status_t;

#ifdef NO_AVX512
#undef __AVX512F__
#undef __AVX512BW__
#undef __AVX512VPOPCNTDQ__
#endif

typedef struct {
    int ftz_enabled;
    int daz_enabled;
} hsd_fp_status_t;

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
int hsd_has_avx512(void);
hsd_fp_status_t hsd_get_fp_mode_status(void);

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
static inline float hsd_internal_hsum_avx_f32(__m256 acc) {
    __m128 sum128 = _mm_add_ps(_mm256_castps256_ps128(acc), _mm256_extractf128_ps(acc, 1));
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    return _mm_cvtss_f32(sum128);
}
#endif

#ifdef __cplusplus
}
#endif

#endif  // HSDLIB_H
