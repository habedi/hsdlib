#include <float.h>
#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>

#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif
#endif

typedef hsd_status_t (*hsd_sqeuclidean_f32_func_t)(const float *, const float *, size_t, float *);

static hsd_status_t sqeuclid_scalar_internal(const float *a, const float *b, size_t n,
                                             float *result) {
    hsd_log("Enter sqeuclid_scalar_internal (n=%zu)", n);
    float sum_sq_diff = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }
    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        *result = sum_sq_diff;
        return HSD_ERR_INVALID_INPUT;
    }
    *result = sum_sq_diff;
    return HSD_SUCCESS;
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx"))) static hsd_status_t sqeuclid_avx_internal(const float *a,
                                                                         const float *b, size_t n,
                                                                         float *result) {
    hsd_log("Enter sqeuclid_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 d = _mm256_sub_ps(va, vb);
#if defined(__FMA__)
        acc = _mm256_fmadd_ps(d, d, acc);
#else
        acc = _mm256_add_ps(acc, _mm256_mul_ps(d, d));
#endif
    }
    float sum_sq_diff = hsd_internal_hsum_avx_f32(acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }
    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        *result = sum_sq_diff;
        return HSD_ERR_INVALID_INPUT;
    }
    *result = sum_sq_diff;
    return HSD_SUCCESS;
}

__attribute__((target("avx2,fma"))) static hsd_status_t sqeuclid_avx2_internal(const float *a,
                                                                               const float *b,
                                                                               size_t n,
                                                                               float *result) {
    hsd_log("Enter sqeuclid_avx2_internal (n=%zu)", n);
    size_t i = 0;
    __m256 acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 d = _mm256_sub_ps(va, vb);
        acc = _mm256_fmadd_ps(d, d, acc);
    }
    float sum_sq_diff = hsd_internal_hsum_avx_f32(acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }
    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        *result = sum_sq_diff;
        return HSD_ERR_INVALID_INPUT;
    }
    *result = sum_sq_diff;
    return HSD_SUCCESS;
}

__attribute__((target("avx512f"))) static hsd_status_t sqeuclid_avx512_internal(const float *a,
                                                                                const float *b,
                                                                                size_t n,
                                                                                float *result) {
    hsd_log("Enter sqeuclid_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 acc = _mm512_setzero_ps();
    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        acc = _mm512_fmadd_ps(_mm512_sub_ps(va, vb), _mm512_sub_ps(va, vb), acc);
    }
    float sum_sq_diff = _mm512_reduce_add_ps(acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        float d = a[i] - b[i];
        sum_sq_diff += d * d;
    }
    if (isnan(sum_sq_diff) || isinf(sum_sq_diff)) {
        *result = sum_sq_diff;
        return HSD_ERR_INVALID_INPUT;
    }
    *result = sum_sq_diff;
    return HSD_SUCCESS;
}
#endif

static hsd_sqeuclidean_f32_func_t resolve_sqeuclidean_f32_internal(void);
static hsd_status_t sqeuclidean_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                        float *result);

static atomic_uintptr_t hsd_sqeuclidean_f32_ptr =
    ATOMIC_VAR_INIT((uintptr_t)sqeuclidean_f32_resolver_trampoline);

hsd_status_t hsd_dist_sqeuclidean_f32(const float *a, const float *b, size_t n, float *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 0.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }
    hsd_sqeuclidean_f32_func_t func = (hsd_sqeuclidean_f32_func_t)atomic_load_explicit(
        &hsd_sqeuclidean_f32_ptr, memory_order_acquire);
    return func(a, b, n, result);
}

static hsd_status_t sqeuclidean_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                        float *result) {
    hsd_log("SqEuclidean F32: resolving backend");
    hsd_sqeuclidean_f32_func_t resolved_func = resolve_sqeuclidean_f32_internal();
    atomic_store_explicit(&hsd_sqeuclidean_f32_ptr, (uintptr_t)resolved_func, memory_order_release);
    return resolved_func(a, b, n, result);
}

static hsd_sqeuclidean_f32_func_t resolve_sqeuclidean_f32_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_sqeuclidean_f32_func_t chosen_func = sqeuclid_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("SqEuclidean F32: Manual backend requested: %d", forced);
        bool supported = false;
        switch (forced) {
            case HSD_BACKEND_AVX512F:
#if defined(__x86_64__) || defined(_M_X64)
                if (hsd_cpu_has_avx512f()) {
                    chosen_func = sqeuclid_avx512_internal;
                    reason = "AVX512F (Forced)";
                    supported = true;
                }
#endif
                break;
            case HSD_BACKEND_AVX2:
#if defined(__x86_64__) || defined(_M_X64)
                if (hsd_cpu_has_avx2()) {
                    chosen_func = sqeuclid_avx2_internal;
                    reason = "AVX2 (Forced)";
                    supported = true;
                } else if (hsd_cpu_has_avx()) {
                    chosen_func = sqeuclid_avx_internal;
                    reason = "AVX (fallback from forced AVX2)";
                    supported = true;
                }
#endif
                break;
            case HSD_BACKEND_AVX:
#if defined(__x86_64__) || defined(_M_X64)
                if (hsd_cpu_has_avx()) {
                    chosen_func = sqeuclid_avx_internal;
                    reason = "AVX (Forced)";
                    supported = true;
                }
#endif
                break;
            case HSD_BACKEND_SCALAR:
                chosen_func = sqeuclid_scalar_internal;
                reason = "Scalar (Forced)";
                supported = true;
                break;
            default:
                reason = "Scalar (Forced backend invalid)";
                break;
        }
        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Warning: Forced backend %d not supported. Falling back to Scalar.", forced);
            chosen_func = sqeuclid_scalar_internal;
            reason = "Scalar (Forced fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f())
            chosen_func = sqeuclid_avx512_internal, reason = "AVX512F (Auto)";
        else if (hsd_cpu_has_avx2())
            chosen_func = sqeuclid_avx2_internal, reason = "AVX2 (Auto)";
        else if (hsd_cpu_has_avx())
            chosen_func = sqeuclid_avx_internal, reason = "AVX (Auto)";
#endif
    }

    hsd_log("Dispatch: Resolved SqEuclidean F32 to: %s", reason);
    return chosen_func;
}
