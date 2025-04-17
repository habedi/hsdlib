#include <math.h>
#include <stddef.h>
#include <stdio.h>

#if defined(__AVX__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif

#include "hsdlib.h"

static inline hsd_status_t euclid_scalar_internal(const float *a, const float *b, size_t n,
                                                  float *result) {
    hsd_log("Enter euclid_scalar_internal (n=%zu)", n);
    float sum_sq_diff = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("Scalar Input Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }

    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        hsd_log("Scalar Sum Check: Sum of squares is NaN or Inf (value: %.8e)", sum_sq_diff);
    }

    *result = sqrtf(sum_sq_diff);

    if (isnan(*result) || isinf(*result)) {
        hsd_log("Scalar Result Check: Final result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }
    hsd_log("Exit euclid_scalar_internal");
    return HSD_SUCCESS;
}

#if defined(__AVX__)

static inline hsd_status_t euclid_avx_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter euclid_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 d = _mm256_sub_ps(va, vb);

        acc = _mm256_add_ps(acc, _mm256_mul_ps(d, d));
    }
    float sum_sq_diff = hsd_internal_hsum_avx_f32(acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }

    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        hsd_log("AVX Sum Check: Sum of squares is NaN or Inf (value: %.8e)", sum_sq_diff);
    }

    *result = sqrtf(sum_sq_diff);

    if (isnan(*result) || isinf(*result)) {
        hsd_log("AVX Result Check: Final result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }
    hsd_log("Exit euclid_avx_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__AVX2__)
static inline hsd_status_t euclid_avx2_internal(const float *a, const float *b, size_t n,
                                                float *result) {
    hsd_log("Enter euclid_avx2_internal (using AVX impl) (n=%zu)", n);
#if defined(__AVX__)

    hsd_status_t status = euclid_avx_internal(a, b, n, result);
    hsd_log("Exit euclid_avx2_internal");
    return status;
#else
    hsd_log("AVX2 defined but AVX not? Falling back to scalar.");
    hsd_status_t status = euclid_scalar_internal(a, b, n, result);
    hsd_log("Exit euclid_avx2_internal");
    return status;
#endif
}
#endif

#if defined(__AVX512F__)
static inline hsd_status_t euclid_avx512_internal(const float *a, const float *b, size_t n,
                                                  float *result) {
    hsd_log("Enter euclid_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 acc = _mm512_setzero_ps();
    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 d = _mm512_sub_ps(va, vb);

        acc = _mm512_add_ps(acc, _mm512_mul_ps(d, d));
    }
    float sum_sq_diff = _mm512_reduce_add_ps(acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX512 Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }

    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        hsd_log("AVX512 Sum Check: Sum of squares is NaN or Inf (value: %.8e)", sum_sq_diff);
    }

    *result = sqrtf(sum_sq_diff);

    if (isnan(*result) || isinf(*result)) {
        hsd_log("AVX512 Result Check: Final result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }
    hsd_log("Exit euclid_avx512_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t euclid_neon_internal(const float *a, const float *b, size_t n,
                                                float *result) {
    hsd_log("Enter euclid_neon_internal (n=%zu)", n);
    size_t i = 0;
    float32x4_t acc = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        float32x4_t d = vsubq_f32(va, vb);

        acc = vfmaq_f32(acc, d, d);
    }

#if defined(__aarch64__)
    float sum_sq_diff = vaddvq_f32(acc);
#else
    float32x2_t sum_pair = vpadd_f32(vget_low_f32(acc), vget_high_f32(acc));
    sum_pair = vpadd_f32(sum_pair, sum_pair);
    float sum_sq_diff = vget_lane_f32(sum_pair, 0);
#endif

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("NEON Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }

    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        hsd_log("NEON Sum Check: Sum of squares is NaN or Inf (value: %.8e)", sum_sq_diff);
    }

    *result = sqrtf(sum_sq_diff);

    if (isnan(*result) || isinf(*result)) {
        hsd_log("NEON Result Check: Final result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }
    hsd_log("Exit euclid_neon_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t euclid_sve_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter euclid_sve_internal (n=%zu)", n);
    int64_t i = 0;
    svbool_t pg;
    svfloat32_t sum_acc = svdup_n_f32(0.0f);

    do {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        svfloat32_t diff = svsub_f32_z(pg, va, vb);

        sum_acc = svmla_f32_z(pg, sum_acc, diff, diff);
        i += svcntw();
    } while (svptest_any(svptrue_b32(), pg));

    float sum_sq_diff = svaddv_f32(svptrue_b32(), sum_acc);

    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        hsd_log("SVE Sum Check: Sum of squares is NaN or Inf (value: %.8e)", sum_sq_diff);
    }

    *result = sqrtf(sum_sq_diff);

    if (isnan(*result) || isinf(*result)) {
        hsd_log("SVE Result Check: Final result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }

    hsd_log("Exit euclid_sve_internal");
    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_euclidean_f32(const float *a, const float *b, size_t n, float *result) {
    hsd_log("Enter hsd_euclidean_f32 (n=%zu)", n);

    if (a == NULL || b == NULL || result == NULL) {
        hsd_log("Input pointers are NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        *result = 0.0f;
        hsd_log("n is 0, returning 0.0f distance.");
        return HSD_SUCCESS;
    }

    hsd_status_t status = HSD_FAILURE;

    hsd_log("Using CPU backend...");

#if defined(__AVX512F__)
    hsd_log("CPU Path: AVX512F");
    status = euclid_avx512_internal(a, b, n, result);
#elif defined(__AVX2__)
    hsd_log("CPU Path: AVX2");
    status = euclid_avx2_internal(a, b, n, result);
#elif defined(__AVX__)
    hsd_log("CPU Path: AVX");
    status = euclid_avx_internal(a, b, n, result);
#elif defined(__ARM_FEATURE_SVE)
    hsd_log("CPU Path: SVE");
    status = euclid_sve_internal(a, b, n, result);
#elif defined(__ARM_NEON)
    hsd_log("CPU Path: NEON");
    status = euclid_neon_internal(a, b, n, result);
#else
    hsd_log("CPU Path: Scalar");
    status = euclid_scalar_internal(a, b, n, result);
#endif

    if (status != HSD_SUCCESS) {
        hsd_log("CPU backend failed (status=%d).", status);
    } else {
        hsd_log("CPU backend succeeded.");
    }

    hsd_log("Exit hsd_euclidean_f32 (final status=%d)", status);
    return status;
}
