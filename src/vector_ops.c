#include <float.h>
#include <math.h>
#include <stddef.h>

#if defined(__AVX__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#include <stdint.h>
#endif

#include "hsdlib.h"

static inline hsd_status_t normalize_l2_scalar_internal(float *vec, size_t n) {
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(vec[i]) || isinf(vec[i])) return HSD_FAILURE;
        sum_sq += (double)vec[i] * (double)vec[i];
    }

    if (isnan(sum_sq) || isinf(sum_sq)) return HSD_FAILURE;

    float norm = sqrtf((float)sum_sq);

    if (isnan(norm)) return HSD_FAILURE;
    if (norm <= FLT_MIN) {
        return HSD_SUCCESS;
    }

    float inv_norm = 1.0f / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) return HSD_FAILURE;

    for (size_t i = 0; i < n; ++i) {
        vec[i] *= inv_norm;
        if (isnan(vec[i])) return HSD_FAILURE;
    }
    return HSD_SUCCESS;
}

#if defined(__AVX__)
static inline hsd_status_t normalize_l2_avx_internal(float *vec, size_t n) {
    size_t i = 0;
    __m256 sum_sq_vec = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 vv = _mm256_loadu_ps(vec + i);
#if defined(__FMA__)
        sum_sq_vec = _mm256_fmadd_ps(vv, vv, sum_sq_vec);
#else
        sum_sq_vec = _mm256_add_ps(sum_sq_vec, _mm256_mul_ps(vv, vv));
#endif
    }
    float sum_sq = hsd_internal_hsum_avx_f32(sum_sq_vec);

    for (; i < n; ++i) {
        if (isnan(vec[i]) || isinf(vec[i])) return HSD_FAILURE;
        sum_sq += vec[i] * vec[i];
    }

    if (isnan(sum_sq) || isinf(sum_sq)) return HSD_FAILURE;

    float norm = sqrtf(sum_sq);
    if (isnan(norm)) return HSD_FAILURE;
    if (norm <= FLT_MIN) return HSD_SUCCESS;

    float inv_norm = 1.0f / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) return HSD_FAILURE;

    i = 0;
    __m256 vinv_norm = _mm256_set1_ps(inv_norm);
    for (; i + 8 <= n; i += 8) {
        __m256 vv = _mm256_loadu_ps(vec + i);
        __m256 vr = _mm256_mul_ps(vv, vinv_norm);
        _mm256_storeu_ps(vec + i, vr);
    }
    for (; i < n; ++i) {
        vec[i] *= inv_norm;
        if (isnan(vec[i])) return HSD_FAILURE;
    }
    return HSD_SUCCESS;
}
#endif

#if defined(__AVX512F__)
static inline hsd_status_t normalize_l2_avx512_internal(float *vec, size_t n) {
    size_t i = 0;
    __m512 sum_sq_vec = _mm512_setzero_ps();
    for (; i + 16 <= n; i += 16) {
        __m512 vv = _mm512_loadu_ps(vec + i);
        sum_sq_vec = _mm512_fmadd_ps(vv, vv, sum_sq_vec);
    }
    float sum_sq = _mm512_reduce_add_ps(sum_sq_vec);

    for (; i < n; ++i) {
        if (isnan(vec[i]) || isinf(vec[i])) return HSD_FAILURE;
        sum_sq += vec[i] * vec[i];
    }

    if (isnan(sum_sq) || isinf(sum_sq)) return HSD_FAILURE;

    float norm = sqrtf(sum_sq);
    if (isnan(norm)) return HSD_FAILURE;
    if (norm <= FLT_MIN) return HSD_SUCCESS;

    float inv_norm = 1.0f / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) return HSD_FAILURE;

    i = 0;
    __m512 vinv_norm = _mm512_set1_ps(inv_norm);
    for (; i + 16 <= n; i += 16) {
        __m512 vv = _mm512_loadu_ps(vec + i);
        __m512 vr = _mm512_mul_ps(vv, vinv_norm);
        _mm512_storeu_ps(vec + i, vr);
    }
    for (; i < n; ++i) {
        vec[i] *= inv_norm;
        if (isnan(vec[i])) return HSD_FAILURE;
    }
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t normalize_l2_neon_internal(float *vec, size_t n) {
    size_t i = 0;
    float32x4_t sum_sq_vec = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) {
        float32x4_t vv = vld1q_f32(vec + i);
        sum_sq_vec = vfmaq_f32(sum_sq_vec, vv, vv);
    }
#if defined(__aarch64__)
    float sum_sq = vaddvq_f32(sum_sq_vec);
#else
    float32x2_t sum_p = vpadd_f32(vget_low_f32(sum_sq_vec), vget_high_f32(sum_sq_vec));
    sum_p = vpadd_f32(sum_p, sum_p);
    float sum_sq = vget_lane_f32(sum_p, 0);
#endif
    for (; i < n; ++i) {
        if (isnan(vec[i]) || isinf(vec[i])) return HSD_FAILURE;
        sum_sq += vec[i] * vec[i];
    }

    if (isnan(sum_sq) || isinf(sum_sq)) return HSD_FAILURE;

    float norm = sqrtf(sum_sq);
    if (isnan(norm)) return HSD_FAILURE;
    if (norm <= FLT_MIN) return HSD_SUCCESS;

    float inv_norm = 1.0f / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) return HSD_FAILURE;

    i = 0;
    float32x4_t vinv_norm = vdupq_n_f32(inv_norm);
    for (; i + 4 <= n; i += 4) {
        float32x4_t vv = vld1q_f32(vec + i);
        float32x4_t vr = vmulq_f32(vv, vinv_norm);
        vst1q_f32(vec + i, vr);
    }
    for (; i < n; ++i) {
        vec[i] *= inv_norm;
        if (isnan(vec[i])) return HSD_FAILURE;
    }
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t normalize_l2_sve_internal(float *vec, size_t n) {
    int64_t i = 0;
    svbool_t pg;
    svfloat32_t sum_sq_vec = svdup_n_f32(0.0f);

    do {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t vv = svld1_f32(pg, vec + i);
        sum_sq_vec = svmla_f32_z(pg, sum_sq_vec, vv, vv);
        i += svcntw();
    } while (svptest_any(svptrue_b32(), pg));

    float sum_sq = svaddv_f32(svptrue_b32(), sum_sq_vec);

    if (isnan(sum_sq) || isinf(sum_sq)) return HSD_FAILURE;

    float norm = sqrtf(sum_sq);
    if (isnan(norm)) return HSD_FAILURE;
    if (norm <= FLT_MIN) return HSD_SUCCESS;

    float inv_norm = 1.0f / norm;
    if (isinf(inv_norm) || isnan(inv_norm)) return HSD_FAILURE;

    i = 0;
    do {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t vv = svld1_f32(pg, vec + i);
        svfloat32_t vr = svmul_f32_z(pg, vv, inv_norm);
        svst1_f32(pg, vec + i, vr);
        i += svcntw();
    } while (svptest_any(svptrue_b32(), pg));

    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_normalize_l2_f32(float *vec, size_t n) {
    if (vec == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) return HSD_SUCCESS;

    hsd_log("Using CPU backend for hsd_normalize_l2_f32: %s", hsd_get_backend());
#if defined(__AVX512F__)
    return normalize_l2_avx512_internal(vec, n);
#elif defined(__AVX__)
    return normalize_l2_avx_internal(vec, n);
#elif defined(__ARM_FEATURE_SVE)
    return normalize_l2_sve_internal(vec, n);
#elif defined(__ARM_NEON)
    return normalize_l2_neon_internal(vec, n);
#else
    return normalize_l2_scalar_internal(vec, n);
#endif
}
