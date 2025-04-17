#include <float.h>
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

static inline hsd_status_t calculate_cosine_distance_from_sums(float dot_product, float norm_a_sq,
                                                               float norm_b_sq, float *result) {
    if (isnan(dot_product) || isnan(norm_a_sq) || isnan(norm_b_sq) || isinf(dot_product) ||
        isinf(norm_a_sq) || isinf(norm_b_sq)) {
        hsd_log("Sums Check: Intermediate sums are Inf/NaN (dot=%.8e, nA_sq=%.8e, nB_sq=%.8e)",
                dot_product, norm_a_sq, norm_b_sq);
        return HSD_FAILURE;
    }

    int a_is_zero = !(norm_a_sq > 0.0f);
    int b_is_zero = !(norm_b_sq > 0.0f);

    if (a_is_zero && b_is_zero) {
        hsd_log("Norm Check: Both norms square <= 0.0f");
        *result = 0.0f;
    } else if (a_is_zero || b_is_zero) {
        hsd_log("Norm Check: One norm square <= 0.0f");
        *result = 1.0f;
    } else {
        float norm_a = sqrtf(norm_a_sq);
        float norm_b = sqrtf(norm_b_sq);

        if (isinf(norm_a) || isinf(norm_b) || isnan(norm_a) || isnan(norm_b) || !(norm_a > 0.0f) ||
            !(norm_b > 0.0f)) {
            hsd_log("Norm Check: sqrt resulted in Inf/NaN/Zero/Negative (norm_a=%.8e, norm_b=%.8e)",
                    norm_a, norm_b);
            return HSD_FAILURE;
        }

        float denominator = norm_a * norm_b;
        float similarity;

        if (denominator <= FLT_MIN) {
            hsd_log("Denominator Check: Denominator %.8e is <= FLT_MIN", denominator);
            similarity = 0.0f;
        } else {
            similarity = dot_product / denominator;
        }

        if (similarity > 1.0f) {
            similarity = 1.0f;
        } else if (similarity < -1.0f) {
            similarity = -1.0f;
        }

        *result = 1.0f - similarity;
    }

    if (isnan(*result) || isinf(*result)) {
        hsd_log("Final Result Check: Result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }

    return HSD_SUCCESS;
}

static inline hsd_status_t cosine_scalar_internal(const float *a, const float *b, size_t n,
                                                  float *result) {
    hsd_log("Enter cosine_scalar_internal (n=%zu)", n);
    float dot_product = 0.0f;
    float norm_a_sq = 0.0f;
    float norm_b_sq = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("Scalar Input Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        dot_product += a[i] * b[i];
        norm_a_sq += a[i] * a[i];
        norm_b_sq += b[i] * b[i];
    }

    hsd_status_t status =
        calculate_cosine_distance_from_sums(dot_product, norm_a_sq, norm_b_sq, result);

    hsd_log("Exit cosine_scalar_internal (status=%d)", status);
    return status;
}

#if defined(__AVX__)

static inline hsd_status_t cosine_avx_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter cosine_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 dot_acc = _mm256_setzero_ps();
    __m256 norm_a_acc = _mm256_setzero_ps();
    __m256 norm_b_acc = _mm256_setzero_ps();

    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);

#if defined(__FMA__)
        dot_acc = _mm256_fmadd_ps(va, vb, dot_acc);
        norm_a_acc = _mm256_fmadd_ps(va, va, norm_a_acc);
        norm_b_acc = _mm256_fmadd_ps(vb, vb, norm_b_acc);
#else
        dot_acc = _mm256_add_ps(dot_acc, _mm256_mul_ps(va, vb));
        norm_a_acc = _mm256_add_ps(norm_a_acc, _mm256_mul_ps(va, va));
        norm_b_acc = _mm256_add_ps(norm_b_acc, _mm256_mul_ps(vb, vb));
#endif
    }

    float dot_product = hsd_internal_hsum_avx_f32(dot_acc);
    float norm_a_sq = hsd_internal_hsum_avx_f32(norm_a_acc);
    float norm_b_sq = hsd_internal_hsum_avx_f32(norm_b_acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        dot_product += a[i] * b[i];
        norm_a_sq += a[i] * a[i];
        norm_b_sq += b[i] * b[i];
    }

    hsd_status_t status =
        calculate_cosine_distance_from_sums(dot_product, norm_a_sq, norm_b_sq, result);

    hsd_log("Exit cosine_avx_internal (status=%d)", status);
    return status;
}
#endif

#if defined(__AVX2__)
static inline hsd_status_t cosine_avx2_internal(const float *a, const float *b, size_t n,
                                                float *result) {
    hsd_log("Enter cosine_avx2_internal (using AVX impl) (n=%zu)", n);
#if defined(__AVX__)

    hsd_status_t status = cosine_avx_internal(a, b, n, result);
    hsd_log("Exit cosine_avx2_internal (status=%d)", status);
    return status;
#else
    hsd_log("AVX2 defined but AVX not? Falling back to scalar.");
    hsd_status_t status = cosine_scalar_internal(a, b, n, result);
    hsd_log("Exit cosine_avx2_internal (status=%d)", status);
    return status;
#endif
}
#endif

#if defined(__AVX512F__)
static inline hsd_status_t cosine_avx512_internal(const float *a, const float *b, size_t n,
                                                  float *result) {
    hsd_log("Enter cosine_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 dot_acc = _mm512_setzero_ps();
    __m512 norm_a_acc = _mm512_setzero_ps();
    __m512 norm_b_acc = _mm512_setzero_ps();

    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        dot_acc = _mm512_fmadd_ps(va, vb, dot_acc);
        norm_a_acc = _mm512_fmadd_ps(va, va, norm_a_acc);
        norm_b_acc = _mm512_fmadd_ps(vb, vb, norm_b_acc);
    }

    float dot_product = _mm512_reduce_add_ps(dot_acc);
    float norm_a_sq = _mm512_reduce_add_ps(norm_a_acc);
    float norm_b_sq = _mm512_reduce_add_ps(norm_b_acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX512 Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        dot_product += a[i] * b[i];
        norm_a_sq += a[i] * a[i];
        norm_b_sq += b[i] * b[i];
    }

    hsd_status_t status =
        calculate_cosine_distance_from_sums(dot_product, norm_a_sq, norm_b_sq, result);

    hsd_log("Exit cosine_avx512_internal (status=%d)", status);
    return status;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t cosine_neon_internal(const float *a, const float *b, size_t n,
                                                float *result) {
    hsd_log("Enter cosine_neon_internal (n=%zu)", n);
    size_t i = 0;
    float32x4_t dot_acc = vdupq_n_f32(0.0f);
    float32x4_t norm_a_acc = vdupq_n_f32(0.0f);
    float32x4_t norm_b_acc = vdupq_n_f32(0.0f);

    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        dot_acc = vfmaq_f32(dot_acc, va, vb);
        norm_a_acc = vfmaq_f32(norm_a_acc, va, va);
        norm_b_acc = vfmaq_f32(norm_b_acc, vb, vb);
    }

#if defined(__aarch64__)
    float dot_product = vaddvq_f32(dot_acc);
    float norm_a_sq = vaddvq_f32(norm_a_acc);
    float norm_b_sq = vaddvq_f32(norm_b_acc);
#else
    float32x2_t dot_p = vpadd_f32(vget_low_f32(dot_acc), vget_high_f32(dot_acc));
    dot_p = vpadd_f32(dot_p, dot_p);
    float dot_product = vget_lane_f32(dot_p, 0);

    float32x2_t na_p = vpadd_f32(vget_low_f32(norm_a_acc), vget_high_f32(norm_a_acc));
    na_p = vpadd_f32(na_p, na_p);
    float norm_a_sq = vget_lane_f32(na_p, 0);

    float32x2_t nb_p = vpadd_f32(vget_low_f32(norm_b_acc), vget_high_f32(norm_b_acc));
    nb_p = vpadd_f32(nb_p, nb_p);
    float norm_b_sq = vget_lane_f32(nb_p, 0);
#endif

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("NEON Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_FAILURE;
        }
        dot_product += a[i] * b[i];
        norm_a_sq += a[i] * a[i];
        norm_b_sq += b[i] * b[i];
    }

    hsd_status_t status =
        calculate_cosine_distance_from_sums(dot_product, norm_a_sq, norm_b_sq, result);

    hsd_log("Exit cosine_neon_internal (status=%d)", status);
    return status;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t cosine_sve_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter cosine_sve_internal (n=%zu)", n);
    int64_t i = 0;
    svbool_t pg;

    svfloat32_t dot_acc = svdup_n_f32(0.0f);
    svfloat32_t norm_a_acc = svdup_n_f32(0.0f);
    svfloat32_t norm_b_acc = svdup_n_f32(0.0f);

    do {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);

        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);

        dot_acc = svmla_f32_z(pg, dot_acc, va, vb);
        norm_a_acc = svmla_f32_z(pg, norm_a_acc, va, va);
        norm_b_acc = svmla_f32_z(pg, norm_b_acc, vb, vb);

        i += svcntw();

    } while (svptest_any(svptrue_b32(), pg));

    float dot_product = svaddv_f32(svptrue_b32(), dot_acc);
    float norm_a_sq = svaddv_f32(svptrue_b32(), norm_a_acc);
    float norm_b_sq = svaddv_f32(svptrue_b32(), norm_b_acc);

    if (isnan(dot_product) || isnan(norm_a_sq) || isnan(norm_b_sq) || isinf(dot_product) ||
        isinf(norm_a_sq) || isinf(norm_b_sq)) {
        hsd_log("SVE Result Check: Reduced sums are NaN/Inf (dot=%.8e, nA_sq=%.8e, nB_sq=%.8e)",
                dot_product, norm_a_sq, norm_b_sq);
        return HSD_FAILURE;
    }

    hsd_status_t status =
        calculate_cosine_distance_from_sums(dot_product, norm_a_sq, norm_b_sq, result);

    hsd_log("Exit cosine_sve_internal (status=%d)", status);
    return status;
}
#endif

hsd_status_t hsd_cosine_f32(const float *a, const float *b, size_t n, float *result) {
    hsd_log("Enter hsd_cosine_f32 (n=%zu)", n);

    if (a == NULL || b == NULL || result == NULL) {
        hsd_log("Input pointers are NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        hsd_log("n is 0, distance is 0.");
        *result = 0.0f;
        return HSD_SUCCESS;
    }

    hsd_status_t status = HSD_FAILURE;

    hsd_log("Using CPU backend...");
#if defined(__AVX512F__)
    hsd_log("CPU Path: AVX512F");
    status = cosine_avx512_internal(a, b, n, result);
#elif defined(__AVX2__)
    hsd_log("CPU Path: AVX2");
    status = cosine_avx2_internal(a, b, n, result);
#elif defined(__AVX__)
    hsd_log("CPU Path: AVX");
    status = cosine_avx_internal(a, b, n, result);
#elif defined(__ARM_FEATURE_SVE)
    hsd_log("CPU Path: SVE");
    status = cosine_sve_internal(a, b, n, result);
#elif defined(__ARM_NEON)
    hsd_log("CPU Path: NEON");
    status = cosine_neon_internal(a, b, n, result);
#else
    hsd_log("CPU Path: Scalar");
    status = cosine_scalar_internal(a, b, n, result);
#endif

    if (status != HSD_SUCCESS) {
        hsd_log("CPU backend failed (status=%d).", status);
    } else {
        hsd_log("CPU backend succeeded.");
    }

    hsd_log("Exit hsd_cosine_f32 (final status=%d)", status);
    return status;
}
