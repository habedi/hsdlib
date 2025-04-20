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

typedef hsd_status_t (*hsd_cosine_f32_func_t)(const float *, const float *, size_t, float *);

static inline hsd_status_t calculate_cosine_similarity_from_sums(float dot_product, float norm_a_sq,
                                                                 float norm_b_sq, float *result) {
    if (isnan(dot_product) || isnan(norm_a_sq) || isnan(norm_b_sq) || isinf(dot_product) ||
        isinf(norm_a_sq) || isinf(norm_b_sq)) {
        hsd_log("Sums Check: Intermediate sums are Inf/NaN");
        *result = NAN;
        return HSD_ERR_INVALID_INPUT;
    }
    int a_zero = (norm_a_sq < FLT_MIN);
    int b_zero = (norm_b_sq < FLT_MIN);
    float similarity;
    if (a_zero && b_zero) {
        similarity = 1.0f;
    } else if (a_zero || b_zero) {
        similarity = 0.0f;
    } else {
        float denom = sqrtf(norm_a_sq) * sqrtf(norm_b_sq);
        if (denom < FLT_MIN)
            similarity = 0.0f;
        else {
            similarity = dot_product / denom;
            if (similarity > 1.0f) similarity = 1.0f;
            if (similarity < -1.0f) similarity = -1.0f;
        }
    }
    *result = similarity;
    return (isnan(similarity) || isinf(similarity)) ? HSD_ERR_INVALID_INPUT : HSD_SUCCESS;
}

static hsd_status_t cosine_scalar_internal(const float *a, const float *b, size_t n,
                                           float *result) {
    hsd_log("Enter cosine_scalar_internal (n=%zu)", n);
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx"))) static hsd_status_t cosine_avx_internal(const float *a,
                                                                       const float *b, size_t n,
                                                                       float *result) {
    hsd_log("Enter cosine_avx_internal (n=%zu)", n);
    size_t i = 0;
    __m256 dot_acc = _mm256_setzero_ps();
    __m256 na_acc = _mm256_setzero_ps();
    __m256 nb_acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
#if defined(__FMA__)
        dot_acc = _mm256_fmadd_ps(va, vb, dot_acc);
        na_acc = _mm256_fmadd_ps(va, va, na_acc);
        nb_acc = _mm256_fmadd_ps(vb, vb, nb_acc);
#else
        dot_acc = _mm256_add_ps(dot_acc, _mm256_mul_ps(va, vb));
        na_acc = _mm256_add_ps(na_acc, _mm256_mul_ps(va, va));
        nb_acc = _mm256_add_ps(nb_acc, _mm256_mul_ps(vb, vb));
#endif
    }
    float dot = hsd_internal_hsum_avx_f32(dot_acc);
    float na = hsd_internal_hsum_avx_f32(na_acc);
    float nb = hsd_internal_hsum_avx_f32(nb_acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}

__attribute__((target("avx2,fma"))) static hsd_status_t cosine_avx2_internal(const float *a,
                                                                             const float *b,
                                                                             size_t n,
                                                                             float *result) {
    hsd_log("Enter cosine_avx2_internal (n=%zu)", n);
    size_t i = 0;
    __m256 dot_acc = _mm256_setzero_ps();
    __m256 na_acc = _mm256_setzero_ps();
    __m256 nb_acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        dot_acc = _mm256_fmadd_ps(va, vb, dot_acc);
        na_acc = _mm256_fmadd_ps(va, va, na_acc);
        nb_acc = _mm256_fmadd_ps(vb, vb, nb_acc);
    }
    float dot = hsd_internal_hsum_avx_f32(dot_acc);
    float na = hsd_internal_hsum_avx_f32(na_acc);
    float nb = hsd_internal_hsum_avx_f32(nb_acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}

__attribute__((target("avx512f"))) static hsd_status_t cosine_avx512_internal(const float *a,
                                                                              const float *b,
                                                                              size_t n,
                                                                              float *result) {
    hsd_log("Enter cosine_avx512_internal (n=%zu)", n);
    size_t i = 0;
    __m512 dot_acc = _mm512_setzero_ps();
    __m512 na_acc = _mm512_setzero_ps();
    __m512 nb_acc = _mm512_setzero_ps();
    for (; i + 16 <= n; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        dot_acc = _mm512_fmadd_ps(va, vb, dot_acc);
        na_acc = _mm512_fmadd_ps(va, va, na_acc);
        nb_acc = _mm512_fmadd_ps(vb, vb, nb_acc);
    }
    float dot = _mm512_reduce_add_ps(dot_acc);
    float na = _mm512_reduce_add_ps(na_acc);
    float nb = _mm512_reduce_add_ps(nb_acc);
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}
#endif

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t cosine_neon_internal(const float *a, const float *b, size_t n, float *result) {
    hsd_log("Enter cosine_neon_internal (n=%zu)", n);
    size_t i = 0;
    float32x4_t dot_acc = vdupq_n_f32(0.0f);
    float32x4_t na_acc = vdupq_n_f32(0.0f);
    float32x4_t nb_acc = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        dot_acc = vfmaq_f32(dot_acc, va, vb);
        na_acc = vfmaq_f32(na_acc, va, va);
        nb_acc = vfmaq_f32(nb_acc, vb, vb);
    }
#if defined(__aarch64__)
    float dot = vaddvq_f32(dot_acc);
    float na = vaddvq_f32(na_acc);
    float nb = vaddvq_f32(nb_acc);
#else
    float32x2_t tmp;
    tmp = vpadd_f32(vget_low_f32(dot_acc), vget_high_f32(dot_acc));
    tmp = vpadd_f32(tmp, tmp);
    float dot = vget_lane_f32(tmp, 0);
    tmp = vpadd_f32(vget_low_f32(na_acc), vget_high_f32(na_acc));
    tmp = vpadd_f32(tmp, tmp);
    float na = vget_lane_f32(tmp, 0);
    tmp = vpadd_f32(vget_low_f32(nb_acc), vget_high_f32(nb_acc));
    tmp = vpadd_f32(tmp, tmp);
    float nb = vget_lane_f32(tmp, 0);
#endif
    for (; i < n; ++i) {
        if (isnan(a[i]) || isnan(b[i]) || isinf(a[i]) || isinf(b[i])) {
            *result = NAN;
            return HSD_ERR_INVALID_INPUT;
        }
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}

#if defined(__ARM_FEATURE_SVE)
__attribute__((target("+sve"))) static hsd_status_t cosine_sve_internal(const float *a,
                                                                        const float *b, size_t n,
                                                                        float *result) {
    hsd_log("Enter cosine_sve_internal (n=%zu)", n);
    int64_t i = 0;
    int64_t n_sve = (int64_t)n;  // Use for loop comparison
    svbool_t pg;
    svfloat32_t dot_acc = svdup_n_f32(0.0f);
    svfloat32_t na_acc = svdup_n_f32(0.0f);
    svfloat32_t nb_acc = svdup_n_f32(0.0f);

    while (i < n_sve) {
        // FIX: Cast loop counter/bound to uint64_t for predicate generation
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)n);

        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);

        // FIX: Use merging predication (_m) for multiply-accumulate
        dot_acc = svmla_f32_m(pg, dot_acc, va, vb);  // dot_acc += va * vb
        na_acc = svmla_f32_m(pg, na_acc, va, va);    // na_acc += va * va
        nb_acc = svmla_f32_m(pg, nb_acc, vb, vb);    // nb_acc += vb * vb

        i += svcntw();
    }
    float dot = svaddv_f32(svptrue_b32(), dot_acc);
    float na = svaddv_f32(svptrue_b32(), na_acc);
    float nb = svaddv_f32(svptrue_b32(), nb_acc);
    return calculate_cosine_similarity_from_sums(dot, na, nb, result);
}
#endif  // __ARM_FEATURE_SVE
#endif

static hsd_cosine_f32_func_t resolve_cosine_f32_internal(void);
static hsd_status_t cosine_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                   float *result);

static atomic_uintptr_t hsd_cosine_f32_ptr =
    ATOMIC_VAR_INIT((uintptr_t)cosine_f32_resolver_trampoline);

hsd_status_t hsd_sim_cosine_f32(const float *a, const float *b, size_t n, float *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 1.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }

    hsd_cosine_f32_func_t func =
        (hsd_cosine_f32_func_t)atomic_load_explicit(&hsd_cosine_f32_ptr, memory_order_acquire);
    return func(a, b, n, result);
}

static hsd_status_t cosine_f32_resolver_trampoline(const float *a, const float *b, size_t n,
                                                   float *result) {
    hsd_cosine_f32_func_t resolved = resolve_cosine_f32_internal();
    uintptr_t expected = (uintptr_t)cosine_f32_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_cosine_f32_ptr, &expected, (uintptr_t)resolved,
                                            memory_order_release, memory_order_relaxed);
    return resolved(a, b, n, result);
}

static hsd_cosine_f32_func_t resolve_cosine_f32_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_cosine_f32_func_t chosen_func = cosine_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Cosine F32: Manual backend requested: %d", forced);
        bool supported = false;
        switch (forced) {
#if defined(__x86_64__) || defined(_M_X64)
            case HSD_BACKEND_AVX512F:
                if (hsd_cpu_has_avx512f()) {
                    chosen_func = cosine_avx512_internal;
                    reason = "AVX512F (Forced)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX2:
                if (hsd_cpu_has_avx2()) {
                    chosen_func = cosine_avx2_internal;
                    reason = "AVX2 (Forced)";
                    supported = true;
                } else if (hsd_cpu_has_avx()) {
                    chosen_func = cosine_avx_internal;
                    reason = "AVX (fallback from forced AVX2)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX:
                if (hsd_cpu_has_avx()) {
                    chosen_func = cosine_avx_internal;
                    reason = "AVX (Forced)";
                    supported = true;
                }
                break;
#endif  // defined(__x86_64__) || defined(_M_X64)

#if defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)
            case HSD_BACKEND_SVE:
                // <<< FIX: Added check
                if (hsd_cpu_has_sve()) {
                    chosen_func = cosine_sve_internal;
                    reason = "SVE (Forced)";
                    supported = true;
                }
                break;
#endif  // SVE
            case HSD_BACKEND_NEON:
                // <<< FIX: Added check
                if (hsd_cpu_has_neon()) {
                    chosen_func = cosine_neon_internal;
                    reason = "NEON (Forced)";
                    supported = true;
                }
                break;
#endif  // defined(__aarch64__) || defined(__arm__)

            case HSD_BACKEND_SCALAR:  // Always available
                chosen_func = cosine_scalar_internal;
                reason = "Scalar (Forced)";
                supported = true;
                break;
            default:  // Unknown forced backend
                reason = "Scalar (Forced backend invalid)";
                chosen_func = cosine_scalar_internal;  // Fallback to scalar
                supported = false;
                break;
        }
        // Fallback if specifically requested backend is not supported by CPU
        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Warning: Forced backend %d not supported. Falling back to Scalar.", forced);
            chosen_func = cosine_scalar_internal;  // Ensure fallback
            reason = "Scalar (Forced backend unsupported)";
        }

    } else {  // AUTO backend selection
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f())
            chosen_func = cosine_avx512_internal, reason = "AVX512F (Auto)";
        else if (hsd_cpu_has_avx2())
            chosen_func = cosine_avx2_internal, reason = "AVX2 (Auto)";
        else if (hsd_cpu_has_avx())
            chosen_func = cosine_avx_internal, reason = "AVX (Auto)";
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)
        if (hsd_cpu_has_sve())
            chosen_func = cosine_sve_internal, reason = "SVE (Auto)";
        else if (hsd_cpu_has_neon())
            chosen_func = cosine_neon_internal, reason = "NEON (Auto)";
#else   // No SVE check needed if SVE feature not compiled
        if (hsd_cpu_has_neon()) chosen_func = cosine_neon_internal, reason = "NEON (Auto)";
#endif  // SVE
#endif  // Architecture check
    }

    hsd_log("Dispatch: Resolved Cosine F32 to: %s", reason);
    return chosen_func;
}
