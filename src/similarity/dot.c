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

typedef hsd_status_t (*hsd_dot_f32_func_t)(const float *, const float *, size_t, float *);

static hsd_status_t dot_scalar_internal(const float *a, const float *b, size_t n, float *result) {
    hsd_log("Enter dot_scalar_internal (n=%zu)", n);
    float dot_product = 0.0f;
    for (size_t i = 0; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        dot_product += a[i] * b[i];
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = dot_product;
    return HSD_SUCCESS;
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx"))) static hsd_status_t dot_avx_internal(const float *a, const float *b,
                                                                    size_t n, float *result) {
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
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        dot_product += a[i] * b[i];
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = dot_product;
    return HSD_SUCCESS;
}

__attribute__((target("avx2,fma"))) static hsd_status_t dot_avx2_internal(const float *a,
                                                                          const float *b, size_t n,
                                                                          float *result) {
    hsd_log("Enter dot_avx2_internal (n=%zu)", n);
    size_t i = 0;
    __m256 dot_acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        dot_acc = _mm256_fmadd_ps(va, vb, dot_acc);
    }
    float dot_product = hsd_internal_hsum_avx_f32(dot_acc);
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        dot_product += a[i] * b[i];
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = dot_product;
    return HSD_SUCCESS;
}

__attribute__((target("avx512f"))) static hsd_status_t dot_avx512_internal(const float *a,
                                                                           const float *b, size_t n,
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
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        dot_product += a[i] * b[i];
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = dot_product;
    return HSD_SUCCESS;
}
#endif

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t dot_neon_internal(const float *a, const float *b, size_t n, float *result) {
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
    float32x2_t tmp = vpadd_f32(vget_low_f32(dot_acc), vget_high_f32(dot_acc));
    tmp = vpadd_f32(tmp, tmp);
    float dot_product = vget_lane_f32(tmp, 0);
#endif
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        dot_product += a[i] * b[i];
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = dot_product;
    return HSD_SUCCESS;
}

#if defined(__ARM_FEATURE_SVE)
__attribute__((target("+sve"))) static hsd_status_t dot_sve_internal(const float *a, const float *b,
                                                                     size_t n, float *result) {
    hsd_log("Enter dot_sve_internal (n=%zu)", n);
    int64_t i = 0;
    int64_t n_sve = (int64_t)n;
    svbool_t pg;
    svfloat32_t dot_acc = svdup_n_f32(0.0f);

    while (i < n_sve) {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        dot_acc = svmla_f32_m(pg, dot_acc, va, vb);
        i += svcntw();
    }

    float dot_product = svaddv_f32(svptrue_b32(), dot_acc);

#if HSD_ALLOW_FP_CHECKS
    if (isnan(dot_product) || isinf(dot_product)) {
        *result = dot_product;
        return HSD_ERR_INVALID_INPUT;
    }
#endif

    *result = dot_product;
    return HSD_SUCCESS;
}
#endif
#endif

static hsd_dot_f32_func_t resolve_dot_f32_internal(void);
static hsd_status_t dot_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                float *result);

static atomic_uintptr_t hsd_dot_f32_ptr = ATOMIC_VAR_INIT((uintptr_t)dot_f32_resolver_trampoline);

hsd_status_t hsd_sim_dot_f32(const float *a, const float *b, size_t n, float *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 0.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }
    hsd_dot_f32_func_t func =
        (hsd_dot_f32_func_t)atomic_load_explicit(&hsd_dot_f32_ptr, memory_order_acquire);
    return func(a, b, n, result);
}

static hsd_status_t dot_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                float *result) {
    hsd_dot_f32_func_t resolved = resolve_dot_f32_internal();
    uintptr_t expected = (uintptr_t)dot_f32_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_dot_f32_ptr, &expected, (uintptr_t)resolved,
                                            memory_order_release, memory_order_relaxed);
    return resolved(a, b, n, result);
}

static hsd_dot_f32_func_t resolve_dot_f32_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_dot_f32_func_t chosen_func = dot_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Dot F32: Manual backend requested: %d", forced);
        bool supported = false;
        switch (forced) {
#if defined(__x86_64__) || defined(_M_X64)
            case HSD_BACKEND_AVX512F:
                if (hsd_cpu_has_avx512f()) {
                    chosen_func = dot_avx512_internal;
                    reason = "AVX512F (Forced)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX2:
                if (hsd_cpu_has_avx2()) {
                    chosen_func = dot_avx2_internal;
                    reason = "AVX2 (Forced)";
                    supported = true;
                } else if (hsd_cpu_has_avx()) {
                    chosen_func = dot_avx_internal;
                    reason = "AVX (fallback from forced AVX2)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX:
                if (hsd_cpu_has_avx()) {
                    chosen_func = dot_avx_internal;
                    reason = "AVX (Forced)";
                    supported = true;
                }
                break;
#elif defined(__aarch64__) || defined(__arm__)
            case HSD_BACKEND_NEON:
                if (hsd_cpu_has_neon()) {
                    chosen_func = dot_neon_internal;
                    reason = "NEON (Forced)";
                    supported = true;
                }
                break;
#if defined(__ARM_FEATURE_SVE)
            case HSD_BACKEND_SVE:
                if (hsd_cpu_has_sve()) {
                    chosen_func = dot_sve_internal;
                    reason = "SVE (Forced)";
                    supported = true;
                }
                break;
#endif
#endif
            case HSD_BACKEND_SCALAR:
                chosen_func = dot_scalar_internal;
                reason = "Scalar (Forced)";
                supported = true;
                break;
            default:
                reason = "Scalar (Forced backend invalid)";
                break;
        }
        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Warning: Forced backend %d not supported. Falling back to Scalar.", forced);
            chosen_func = dot_scalar_internal;
            reason = "Scalar (Forced fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f()) {
            chosen_func = dot_avx512_internal;
            reason = "AVX512F (Auto)";
        } else if (hsd_cpu_has_avx2()) {
            chosen_func = dot_avx2_internal;
            reason = "AVX2 (Auto)";
        } else if (hsd_cpu_has_avx()) {
            chosen_func = dot_avx_internal;
            reason = "AVX (Auto)";
        }
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)
        if (hsd_cpu_has_sve()) {
            chosen_func = dot_sve_internal;
            reason = "SVE (Auto)";
        } else if (hsd_cpu_has_neon()) {
            chosen_func = dot_neon_internal;
            reason = "NEON (Auto)";
        }
#else
        if (hsd_cpu_has_neon()) {
            chosen_func = dot_neon_internal;
            reason = "NEON (Auto)";
        }
#endif
#endif
    }

    hsd_log("Dispatch: Resolved Dot F32 to: %s", reason);
    return chosen_func;
}
