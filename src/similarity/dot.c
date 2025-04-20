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

static inline hsd_status_t dot_scalar_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter dot_scalar_internal (n=%zu)", n);
    float dot_product = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("Scalar Input Check: NaN/Inf detected at index %zu", i);
            return HSD_ERR_INVALID_INPUT;
        }
        dot_product += a[i] * b[i];
    }

    if (isnan(dot_product) || isinf(dot_product)) {
        hsd_log("Scalar Result Check: Final dot product is NaN or Inf (value: %.8e)", dot_product);
        return HSD_ERR_INVALID_INPUT;
    }

    *result = dot_product;
    hsd_log("Exit dot_scalar_internal");
    return HSD_SUCCESS;
}

#if defined(__AVX__)
static inline hsd_status_t dot_avx_internal(const float *a, const float *b, size_t n,
                                            float *result) {
    hsd_log("Enter dot_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 dot_acc = _mm256_setzero_ps();

    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
#if defined(__FMA__)
        dot_acc = _mm256_fmadd_ps(va, vb, dot_acc);
#else
        dot_acc = _mm256_add_ps(dot_acc, _mm256_mul_ps(va, vb));
#endif
    }

    float dot_product = hsd_internal_hsum_avx_f32(dot_acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_ERR_INVALID_INPUT;
        }
        dot_product += a[i] * b[i];
    }

    if (isnan(dot_product) || isinf(dot_product)) {
        hsd_log("AVX Result Check: Final dot product is NaN or Inf (value: %.8e)", dot_product);
        return HSD_ERR_INVALID_INPUT;
    }

    *result = dot_product;
    hsd_log("Exit dot_avx_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__AVX2__)
static inline hsd_status_t dot_avx2_internal(const float *a, const float *b, size_t n,
                                             float *result) {
    hsd_log("Enter dot_avx2_internal (using AVX impl) (n=%zu)", n);
#if defined(__AVX__)
    hsd_status_t status = dot_avx_internal(a, b, n, result);
    hsd_log("Exit dot_avx2_internal (status=%d)", status);
    return status;
#else
    hsd_log("AVX2 defined but AVX not? Falling back to scalar.");
    hsd_status_t status = dot_scalar_internal(a, b, n, result);
    hsd_log("Exit dot_avx2_internal (status=%d)", status);
    return status;
#endif
}
#endif

#if defined(__AVX512F__)
static inline hsd_status_t dot_avx512_internal(const float *a, const float *b, size_t n,
                                               float *result) {
    hsd_log("Enter dot_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 dot_acc = _mm512_setzero_ps();

    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        dot_acc = _mm512_fmadd_ps(va, vb, dot_acc);
    }

    float dot_product = _mm512_reduce_add_ps(dot_acc);

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("AVX512 Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_ERR_INVALID_INPUT;
        }
        dot_product += a[i] * b[i];
    }

    if (isnan(dot_product) || isinf(dot_product)) {
        hsd_log("AVX512 Result Check: Final dot product is NaN or Inf (value: %.8e)", dot_product);
        return HSD_ERR_INVALID_INPUT;
    }

    *result = dot_product;
    hsd_log("Exit dot_avx512_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t dot_neon_internal(const float *a, const float *b, size_t n,
                                             float *result) {
    hsd_log("Enter dot_neon_internal (n=%zu)", n);
    size_t i = 0;
    float32x4_t dot_acc = vdupq_n_f32(0.0f);

    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        dot_acc = vfmaq_f32(dot_acc, va, vb);
    }

#if defined(__aarch64__)
    float dot_product = vaddvq_f32(dot_acc);
#else
    float32x2_t dot_p = vpadd_f32(vget_low_f32(dot_acc), vget_high_f32(dot_acc));
    dot_p = vpadd_f32(dot_p, dot_p);
    float dot_product = vget_lane_f32(dot_p, 0);
#endif

    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            hsd_log("NEON Remainder Check: NaN/Inf detected at index %zu", i);
            return HSD_ERR_INVALID_INPUT;
        }
        dot_product += a[i] * b[i];
    }

    if (isnan(dot_product) || isinf(dot_product)) {
        hsd_log("NEON Result Check: Final dot product is NaN or Inf (value: %.8e)", dot_product);
        return HSD_ERR_INVALID_INPUT;
    }

    *result = dot_product;
    hsd_log("Exit dot_neon_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t dot_sve_internal(const float *a, const float *b, size_t n,
                                            float *result) {
    hsd_log("Enter dot_sve_internal (n=%zu)", n);
    int64_t i = 0;
    svbool_t pg;
    svfloat32_t dot_acc = svdup_n_f32(0.0f);

    do {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        dot_acc = svmla_f32_z(pg, dot_acc, va, vb);
        i += svcntw();
    } while (svptest_any(svptrue_b32(), pg));

    float dot_product = svaddv_f32(svptrue_b32(), dot_acc);

    if (isnan(dot_product) || isinf(dot_product)) {
        hsd_log("SVE Result Check: Final dot product is NaN or Inf (value: %.8e)", dot_product);
        return HSD_ERR_INVALID_INPUT;
    }

    *result = dot_product;
    hsd_log("Exit dot_sve_internal");
    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_sim_dot_f32(const float *a, const float *b, size_t n, float *result) {
    hsd_log("Enter hsd_sim_dot_f32 (n=%zu)", n);

    if (result == NULL) {
        hsd_log("Result pointer is NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        hsd_log("n is 0, dot product is 0.");
        *result = 0.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        hsd_log("Input array pointers are NULL for non-zero n!");
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }

    hsd_status_t status = HSD_FAILURE;

#if defined(HSD_TARGET_AVX512)
    hsd_log("CPU Path: Forced AVX512F");
#if defined(__AVX512F__)
    status = dot_avx512_internal(a, b, n, result);
#else
#error "HSD_TARGET_AVX512 requires compiler support for AVX512F (e.g., -mavx512f)"
    *result = NAN;
    status = HSD_ERR_UNSUPPORTED;
#endif
#elif defined(HSD_TARGET_AVX2)
    hsd_log("CPU Path: Forced AVX2");
#if defined(__AVX2__)
    status = dot_avx2_internal(a, b, n, result);
#else
#error "HSD_TARGET_AVX2 requires compiler support for AVX2 (e.g., -mavx2)"
    *result = NAN;
    status = HSD_ERR_UNSUPPORTED;
#endif
#elif defined(HSD_TARGET_AVX)
    hsd_log("CPU Path: Forced AVX");
#if defined(__AVX__)
    status = dot_avx_internal(a, b, n, result);
#else
#error "HSD_TARGET_AVX requires compiler support for AVX (e.g., -mavx)"
    *result = NAN;
    status = HSD_ERR_UNSUPPORTED;
#endif
#elif defined(HSD_TARGET_SVE)
    hsd_log("CPU Path: Forced SVE");
#if defined(__ARM_FEATURE_SVE)
    status = dot_sve_internal(a, b, n, result);
#else
#error "HSD_TARGET_SVE requires compiler support for SVE (e.g., -march=armv8.2-a+sve)"
    *result = NAN;
    status = HSD_ERR_UNSUPPORTED;
#endif
#elif defined(HSD_TARGET_NEON)
    hsd_log("CPU Path: Forced NEON");
#if defined(__ARM_NEON)
    status = dot_neon_internal(a, b, n, result);
#else
#error "HSD_TARGET_NEON requires compiler support for NEON (e.g., -mfpu=neon)"
    *result = NAN;
    status = HSD_ERR_UNSUPPORTED;
#endif
#elif defined(HSD_TARGET_SCALAR)
    hsd_log("CPU Path: Forced Scalar");
    status = dot_scalar_internal(a, b, n, result);
#else
    hsd_log("Using CPU backend (auto-detected)...");
#if defined(__AVX512F__)
    hsd_log("CPU Path: Auto AVX512F");
    status = dot_avx512_internal(a, b, n, result);
#elif defined(__AVX2__)
    hsd_log("CPU Path: Auto AVX2");
    status = dot_avx2_internal(a, b, n, result);
#elif defined(__AVX__)
    hsd_log("CPU Path: Auto AVX");
    status = dot_avx_internal(a, b, n, result);
#elif defined(__ARM_FEATURE_SVE)
    hsd_log("CPU Path: Auto SVE");
    status = dot_sve_internal(a, b, n, result);
#elif defined(__ARM_NEON)
    hsd_log("CPU Path: Auto NEON");
    status = dot_neon_internal(a, b, n, result);
#else
    hsd_log("CPU Path: Auto Scalar");
    status = dot_scalar_internal(a, b, n, result);
#endif
#endif

    if (status != HSD_SUCCESS && status != HSD_ERR_UNSUPPORTED) {
        hsd_log("CPU backend failed (status=%d).", status);
    } else if (status == HSD_SUCCESS) {
        hsd_log("CPU backend succeeded. Dot product: %f", *result);
    }

    hsd_log("Exit hsd_sim_dot_f32 (final status=%d)", status);
    return status;
}
