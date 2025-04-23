/*
 * If HSDLIB_NO_CHECKS is defined, all isnan/isinf tests
 * get compiled out for maximum speed.
 */
#ifdef HSDLIB_NO_CHECKS
#define HSD_ALLOW_FP_CHECKS 0
#else
#define HSD_ALLOW_FP_CHECKS 1
#endif

#include <float.h>
#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
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

typedef hsd_status_t (*hsd_manhattan_f32_func_t)(const float *, const float *, size_t, float *);

static hsd_status_t manhattan_scalar_internal(const float *a, const float *b, size_t n,
                                              float *result) {
    hsd_log("Enter manhattan_scalar_internal (n=%zu)", n);
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        sum += fabsf(a[i] - b[i]);
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx"))) static hsd_status_t manhattan_avx_internal(const float *a,
                                                                          const float *b, size_t n,
                                                                          float *result) {
    hsd_log("Enter manhattan_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 acc = _mm256_setzero_ps();
    const __m256 abs_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        __m256 ad = _mm256_and_ps(diff, abs_mask);
        acc = _mm256_add_ps(acc, ad);
    }
    float sum = hsd_internal_hsum_avx_f32(acc);
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        sum += fabsf(a[i] - b[i]);
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}

__attribute__((target("avx2"))) static hsd_status_t manhattan_avx2_internal(const float *a,
                                                                            const float *b,
                                                                            size_t n,
                                                                            float *result) {
    hsd_log("Enter manhattan_avx2_internal (n=%zu)", n);
    size_t i = 0;
    __m256 acc = _mm256_setzero_ps();
    const __m256 abs_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        __m256 ad = _mm256_and_ps(diff, abs_mask);
        acc = _mm256_add_ps(acc, ad);
    }
    float sum = hsd_internal_hsum_avx_f32(acc);
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        sum += fabsf(a[i] - b[i]);
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}

__attribute__((target("avx512f"))) static hsd_status_t manhattan_avx512_internal(const float *a,
                                                                                 const float *b,
                                                                                 size_t n,
                                                                                 float *result) {
    hsd_log("Enter manhattan_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 acc = _mm512_setzero_ps();
    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 diff = _mm512_sub_ps(va, vb);
        acc = _mm512_add_ps(acc, _mm512_abs_ps(diff));
    }
    float sum = _mm512_reduce_add_ps(acc);
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        sum += fabsf(a[i] - b[i]);
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}
#endif

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t manhattan_neon_internal(const float *a, const float *b, size_t n,
                                            float *result) {
    hsd_log("Enter manhattan_neon_internal (n=%zu)", n);
    size_t i = 0;
    float32x4_t acc = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        float32x4_t diff = vsubq_f32(va, vb);
        acc = vaddq_f32(acc, vabsq_f32(diff));
    }
#if defined(__aarch64__)
    float sum = vaddvq_f32(acc);
#else
    float32x2_t p = vpadd_f32(vget_low_f32(acc), vget_high_f32(acc));
    p = vpadd_f32(p, p);
    float sum = vget_lane_f32(p, 0);
#endif
    for (; i < n; ++i) {
#if HSD_ALLOW_FP_CHECKS
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
#endif
        sum += fabsf(a[i] - b[i]);
    }
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}

#if defined(__ARM_FEATURE_SVE)
__attribute__((target("+sve"))) static hsd_status_t manhattan_sve_internal(const float *a,
                                                                           const float *b, size_t n,
                                                                           float *result) {
    hsd_log("Enter manhattan_sve_internal (n=%zu)", n);
    int64_t i = 0;
    int64_t n_sve = (int64_t)n;
    svbool_t pg;
    svfloat32_t acc = svdup_n_f32(0.0f);

    while (i < n_sve) {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        svfloat32_t diff = svsub_f32_z(pg, va, vb);
        svfloat32_t ad = svabs_f32_z(pg, diff);
        acc = svadd_f32_m(pg, acc, ad);
        i += svcntw();
    }

    float sum = svaddv_f32(svptrue_b32(), acc);
#if HSD_ALLOW_FP_CHECKS
    if (isnan(sum) || isinf(sum)) {
        *result = sum;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    *result = sum;
    return HSD_SUCCESS;
}
#endif
#endif

static hsd_manhattan_f32_func_t resolve_manhattan_f32_internal(void);
static hsd_status_t manhattan_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                      float *result);

static atomic_uintptr_t hsd_manhattan_f32_ptr =
    ATOMIC_VAR_INIT((uintptr_t)manhattan_f32_resolver_trampoline);

hsd_status_t hsd_dist_manhattan_f32(const float *a, const float *b, size_t n, float *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 0.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }
    hsd_manhattan_f32_func_t func = (hsd_manhattan_f32_func_t)atomic_load_explicit(
        &hsd_manhattan_f32_ptr, memory_order_acquire);
    return func(a, b, n, result);
}

static hsd_status_t manhattan_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                      float *result) {
    hsd_manhattan_f32_func_t resolved = resolve_manhattan_f32_internal();
    uintptr_t exp = (uintptr_t)manhattan_f32_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_manhattan_f32_ptr, &exp, (uintptr_t)resolved,
                                            memory_order_release, memory_order_relaxed);
    return resolved(a, b, n, result);
}

static hsd_manhattan_f32_func_t resolve_manhattan_f32_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_manhattan_f32_func_t chosen = manhattan_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Manhattan F32: Forced backend %d", forced);
        bool supported = false;
#if defined(__x86_64__) || defined(_M_X64)
        if (forced == HSD_BACKEND_AVX512F && hsd_cpu_has_avx512f()) {
            chosen = manhattan_avx512_internal;
            reason = "AVX512F (Forced)";
            supported = true;
        } else if (forced == HSD_BACKEND_AVX2 && hsd_cpu_has_avx2()) {
            chosen = manhattan_avx2_internal;
            reason = "AVX2 (Forced)";
            supported = true;
        } else if (forced == HSD_BACKEND_AVX && hsd_cpu_has_avx()) {
            chosen = manhattan_avx_internal;
            reason = "AVX (Forced)";
            supported = true;
        }
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)

        if (forced == HSD_BACKEND_SVE && hsd_cpu_has_sve()) {
            chosen = manhattan_sve_internal;
            reason = "SVE (Forced)";
            supported = true;
        } else
#endif
            if (forced == HSD_BACKEND_NEON && hsd_cpu_has_neon()) {
            chosen = manhattan_neon_internal;
            reason = "NEON (Forced)";
            supported = true;
        }
#endif
        else if (forced == HSD_BACKEND_SCALAR) {
            chosen = manhattan_scalar_internal;
            reason = "Scalar (Forced)";
            supported = true;
        }

        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Forced backend %d not supported; falling back", forced);
            chosen = manhattan_scalar_internal;
            reason = "Scalar (Fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f())
            chosen = manhattan_avx512_internal, reason = "AVX512F (Auto)";
        else if (hsd_cpu_has_avx2())
            chosen = manhattan_avx2_internal, reason = "AVX2 (Auto)";
        else if (hsd_cpu_has_avx())
            chosen = manhattan_avx_internal, reason = "AVX (Auto)";
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)

        if (hsd_cpu_has_sve())
            chosen = manhattan_sve_internal, reason = "SVE (Auto)";
        else
#endif
            if (hsd_cpu_has_neon())
            chosen = manhattan_neon_internal, reason = "NEON (Auto)";
#endif
    }

    hsd_log("Dispatch: Resolved Manhattan F32 to: %s", reason);
    return chosen;
}
