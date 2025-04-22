#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
extern bool hsd_cpu_has_avx512f(void);
extern bool hsd_cpu_has_avx512bw(void);
extern bool hsd_cpu_has_avx512dq(void);
extern bool hsd_cpu_has_avx2(void);
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
extern bool hsd_cpu_has_neon(void);
#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
extern bool hsd_cpu_has_sve(void);
#endif
#endif

typedef struct {
    uint64_t dot_product;
    uint64_t norm_a_sq;
    uint64_t norm_b_sq;
} HSD_TripleSumU64;

typedef hsd_status_t (*hsd_jaccard_get_sums_func_t)(const uint16_t *, const uint16_t *, size_t,
                                                    HSD_TripleSumU64 *);

static inline hsd_status_t calculate_jaccard_similarity_from_sums_u64(uint64_t dot, uint64_t nAsq,
                                                                      uint64_t nBsq,
                                                                      float *result) {
    if (nAsq == 0 && nBsq == 0) {
        *result = 1.0f;
        return HSD_SUCCESS;
    }
    double d_dot = (double)dot;
    double d_nAsq = (double)nAsq;
    double d_nBsq = (double)nBsq;
    double denom = d_nAsq + d_nBsq - d_dot;
    double sim;

    if (denom < 1e-9) {
        sim = 1.0;
    } else {
        sim = d_dot / denom;
    }

    if (sim > 1.0) sim = 1.0;
    if (sim < 0.0) sim = 0.0;

    *result = (float)sim;

#if HSD_ALLOW_FP_CHECKS
    if (isnan(*result) || isinf(*result)) {
        *result = NAN;
        return HSD_ERR_INVALID_INPUT;
    }
#endif
    return HSD_SUCCESS;
}

static hsd_status_t jaccard_get_sums_scalar_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                     HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_scalar_internal<u16> (n=%zu)", n);
    uint64_t dot_p = 0, n_a_sq = 0, n_b_sq = 0;
    for (size_t i = 0; i < n; ++i) {
        uint64_t va = a[i], vb = b[i];
        dot_p += va * vb;
        n_a_sq += va * va;
        n_b_sq += vb * vb;
    }

    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;

    return HSD_SUCCESS;
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx2"))) static hsd_status_t jaccard_get_sums_avx2_internal(
    const uint16_t *a, const uint16_t *b, size_t n, HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_avx2_internal<u16> (n=%zu)", n);
    size_t i = 0;
    __m256i dot_acc = _mm256_setzero_si256();
    __m256i a_acc = _mm256_setzero_si256();
    __m256i b_acc = _mm256_setzero_si256();

    for (; i + 16 <= n; i += 16) {
        __m256i va16 = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb16 = _mm256_loadu_si256((const __m256i *)(b + i));
        __m128i va_lo = _mm256_castsi256_si128(va16);
        __m128i va_hi = _mm256_extracti128_si256(va16, 1);
        __m128i vb_lo = _mm256_castsi256_si128(vb16);
        __m128i vb_hi = _mm256_extracti128_si256(vb16, 1);

        __m256i va32_lo = _mm256_cvtepu16_epi32(va_lo);
        __m256i va32_hi = _mm256_cvtepu16_epi32(va_hi);
        __m256i vb32_lo = _mm256_cvtepu16_epi32(vb_lo);
        __m256i vb32_hi = _mm256_cvtepu16_epi32(vb_hi);

        __m256i dot_lo = _mm256_mullo_epi32(va32_lo, vb32_lo);
        __m256i dot_hi = _mm256_mullo_epi32(va32_hi, vb32_hi);
        __m256i a_lo2 = _mm256_mullo_epi32(va32_lo, va32_lo);
        __m256i a_hi2 = _mm256_mullo_epi32(va32_hi, va32_hi);
        __m256i b_lo2 = _mm256_mullo_epi32(vb32_lo, vb32_lo);
        __m256i b_hi2 = _mm256_mullo_epi32(vb32_hi, vb32_hi);

        __m256i dot_p0 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(dot_lo));
        __m256i dot_p1 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot_lo, 1));
        __m256i dot_p2 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(dot_hi));
        __m256i dot_p3 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot_hi, 1));
        dot_acc = _mm256_add_epi64(dot_acc, dot_p0);
        dot_acc = _mm256_add_epi64(dot_acc, dot_p1);
        dot_acc = _mm256_add_epi64(dot_acc, dot_p2);
        dot_acc = _mm256_add_epi64(dot_acc, dot_p3);

        __m256i a_p0 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(a_lo2));
        __m256i a_p1 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(a_lo2, 1));
        __m256i a_p2 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(a_hi2));
        __m256i a_p3 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(a_hi2, 1));
        a_acc = _mm256_add_epi64(a_acc, a_p0);
        a_acc = _mm256_add_epi64(a_acc, a_p1);
        a_acc = _mm256_add_epi64(a_acc, a_p2);
        a_acc = _mm256_add_epi64(a_acc, a_p3);

        __m256i b_p0 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(b_lo2));
        __m256i b_p1 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(b_lo2, 1));
        __m256i b_p2 = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(b_hi2));
        __m256i b_p3 = _mm256_cvtepu32_epi64(_mm256_extracti128_si256(b_hi2, 1));
        b_acc = _mm256_add_epi64(b_acc, b_p0);
        b_acc = _mm256_add_epi64(b_acc, b_p1);
        b_acc = _mm256_add_epi64(b_acc, b_p2);
        b_acc = _mm256_add_epi64(b_acc, b_p3);
    }

    uint64_t dot_s[4], a_s[4], b_s[4];
    _mm256_storeu_si256((__m256i *)dot_s, dot_acc);
    _mm256_storeu_si256((__m256i *)a_s, a_acc);
    _mm256_storeu_si256((__m256i *)b_s, b_acc);

    uint64_t dot_p = dot_s[0] + dot_s[1] + dot_s[2] + dot_s[3];
    uint64_t n_a_sq = a_s[0] + a_s[1] + a_s[2] + a_s[3];
    uint64_t n_b_sq = b_s[0] + b_s[1] + b_s[2] + b_s[3];

    for (; i < n; ++i) {
        uint64_t va = a[i], vb = b[i];
        dot_p += va * vb;
        n_a_sq += va * va;
        n_b_sq += vb * vb;
    }

    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    return HSD_SUCCESS;
}

__attribute__((target("avx512f,avx512bw,avx512dq"))) static hsd_status_t
jaccard_get_sums_avx512_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                 HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_avx512_internal<u16> (n=%zu)", n);
    size_t i = 0;
    __m512i dot_acc = _mm512_setzero_si512();
    __m512i a_acc = _mm512_setzero_si512();
    __m512i b_acc = _mm512_setzero_si512();

    for (; i + 32 <= n; i += 32) {
        __m512i va16 = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb16 = _mm512_loadu_si512((const __m512i *)(b + i));

        __m256i va16_lo = _mm512_extracti64x4_epi64(va16, 0);
        __m256i va16_hi = _mm512_extracti64x4_epi64(va16, 1);
        __m256i vb16_lo = _mm512_extracti64x4_epi64(vb16, 0);
        __m256i vb16_hi = _mm512_extracti64x4_epi64(vb16, 1);

        __m512i va32_lo = _mm512_cvtepu16_epi32(va16_lo);
        __m512i va32_hi = _mm512_cvtepu16_epi32(va16_hi);
        __m512i vb32_lo = _mm512_cvtepu16_epi32(vb16_lo);
        __m512i vb32_hi = _mm512_cvtepu16_epi32(vb16_hi);

        __m512i dot_lo = _mm512_mullo_epi32(va32_lo, vb32_lo);
        __m512i dot_hi = _mm512_mullo_epi32(va32_hi, vb32_hi);
        __m512i a_lo2 = _mm512_mullo_epi32(va32_lo, va32_lo);
        __m512i a_hi2 = _mm512_mullo_epi32(va32_hi, va32_hi);
        __m512i b_lo2 = _mm512_mullo_epi32(vb32_lo, vb32_lo);
        __m512i b_hi2 = _mm512_mullo_epi32(vb32_hi, vb32_hi);

        dot_acc =
            _mm512_add_epi64(dot_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot_lo, 0)));
        dot_acc =
            _mm512_add_epi64(dot_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot_lo, 1)));
        dot_acc =
            _mm512_add_epi64(dot_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot_hi, 0)));
        dot_acc =
            _mm512_add_epi64(dot_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot_hi, 1)));

        a_acc = _mm512_add_epi64(a_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(a_lo2, 0)));
        a_acc = _mm512_add_epi64(a_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(a_lo2, 1)));
        a_acc = _mm512_add_epi64(a_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(a_hi2, 0)));
        a_acc = _mm512_add_epi64(a_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(a_hi2, 1)));

        b_acc = _mm512_add_epi64(b_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(b_lo2, 0)));
        b_acc = _mm512_add_epi64(b_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(b_lo2, 1)));
        b_acc = _mm512_add_epi64(b_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(b_hi2, 0)));
        b_acc = _mm512_add_epi64(b_acc, _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(b_hi2, 1)));
    }

    uint64_t dot_p = _mm512_reduce_add_epi64(dot_acc);
    uint64_t n_a_sq = _mm512_reduce_add_epi64(a_acc);
    uint64_t n_b_sq = _mm512_reduce_add_epi64(b_acc);

    for (; i < n; ++i) {
        uint64_t va = a[i], vb = b[i];
        dot_p += va * vb;
        n_a_sq += va * va;
        n_b_sq += vb * vb;
    }

    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    return HSD_SUCCESS;
}
#endif /* __x86_64__ */

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t jaccard_get_sums_neon_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                   HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_neon_internal<u16> (n=%zu)", n);
    size_t i = 0;
    uint64x2_t dot_acc = vdupq_n_u64(0);
    uint64x2_t a_acc = vdupq_n_u64(0);
    uint64x2_t b_acc = vdupq_n_u64(0);

    for (; i + 8 <= n; i += 8) {
        uint16x8_t va16 = vld1q_u16(a + i);
        uint16x8_t vb16 = vld1q_u16(b + i);

        uint32x4_t dot_lo = vmull_u16(vget_low_u16(va16), vget_low_u16(vb16));
        uint32x4_t dot_hi = vmull_u16(vget_high_u16(va16), vget_high_u16(vb16));
        uint32x4_t a_lo2 = vmull_u16(vget_low_u16(va16), vget_low_u16(va16));
        uint32x4_t a_hi2 = vmull_u16(vget_high_u16(va16), vget_high_u16(va16));
        uint32x4_t b_lo2 = vmull_u16(vget_low_u16(vb16), vget_low_u16(vb16));
        uint32x4_t b_hi2 = vmull_u16(vget_high_u16(vb16), vget_high_u16(vb16));

        dot_acc = vpadalq_u32(dot_acc, dot_lo);
        dot_acc = vpadalq_u32(dot_acc, dot_hi);
        a_acc = vpadalq_u32(a_acc, a_lo2);
        a_acc = vpadalq_u32(a_acc, a_hi2);
        b_acc = vpadalq_u32(b_acc, b_lo2);
        b_acc = vpadalq_u32(b_acc, b_hi2);
    }

#if defined(__aarch64__)
    uint64_t dot_p = vaddvq_u64(dot_acc);
    uint64_t n_a_sq = vaddvq_u64(a_acc);
    uint64_t n_b_sq = vaddvq_u64(b_acc);
#else
    uint64_t dot_p = vgetq_lane_u64(dot_acc, 0) + vgetq_lane_u64(dot_acc, 1);
    uint64_t n_a_sq = vgetq_lane_u64(a_acc, 0) + vgetq_lane_u64(a_acc, 1);
    uint64_t n_b_sq = vgetq_lane_u64(b_acc, 0) + vgetq_lane_u64(b_acc, 1);
#endif

    for (; i < n; ++i) {
        uint64_t va = a[i], vb = b[i];
        dot_p += va * vb;
        n_a_sq += va * va;
        n_b_sq += vb * vb;
    }

    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    return HSD_SUCCESS;
}

#if defined(__ARM_FEATURE_SVE)
__attribute__((target("+sve"))) static hsd_status_t jaccard_get_sums_sve_internal(
    const uint16_t *a, const uint16_t *b, size_t n, HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_sve_internal<u16> (n=%zu)", n);
    int64_t i = 0;
    int64_t n_sve = (int64_t)n;
    svbool_t pg;
    svuint64_t dot_acc = svdup_n_u64(0);
    svuint64_t a_acc = svdup_n_u64(0);
    svuint64_t b_acc = svdup_n_u64(0);

    while (i < n_sve) {
        pg = svwhilelt_b16((uint64_t)i, (uint64_t)n_sve);
        svuint16_t va16 = svld1_u16(pg, a + i);
        svuint16_t vb16 = svld1_u16(pg, b + i);

        svuint32_t va_lo = svunpklo_u32(va16);
        svuint32_t va_hi = svunpkhi_u32(va16);
        svuint32_t vb_lo = svunpklo_u32(vb16);
        svuint32_t vb_hi = svunpkhi_u32(vb16);

        svuint32_t dot_lo = svmul_u32_z(svptrue_b32(), va_lo, vb_lo);

        svuint32_t dot_hi = svmul_u32_z(svptrue_b32(), va_hi, vb_hi);
        svuint32_t a_lo2 = svmul_u32_z(svptrue_b32(), va_lo, va_lo);
        svuint32_t a_hi2 = svmul_u32_z(svptrue_b32(), va_hi, va_hi);
        svuint32_t b_lo2 = svmul_u32_z(svptrue_b32(), vb_lo, vb_lo);
        svuint32_t b_hi2 = svmul_u32_z(svptrue_b32(), vb_hi, vb_hi);

        dot_acc = svaddwb_u64(dot_acc, dot_lo);
        dot_acc = svaddwt_u64(dot_acc, dot_hi);
        a_acc = svaddwb_u64(a_acc, a_lo2);
        a_acc = svaddwt_u64(a_acc, a_hi2);
        b_acc = svaddwb_u64(b_acc, b_lo2);
        b_acc = svaddwt_u64(b_acc, b_hi2);

        i += svcnth();
    }

    sums->dot_product = svaddv_u64(svptrue_b64(), dot_acc);
    sums->norm_a_sq = svaddv_u64(svptrue_b64(), a_acc);
    sums->norm_b_sq = svaddv_u64(svptrue_b64(), b_acc);
    return HSD_SUCCESS;
}
#endif
#endif

static hsd_jaccard_get_sums_func_t resolve_jaccard_get_sums_internal(void);
static hsd_status_t jaccard_get_sums_resolver_trampoline(const uint16_t *, const uint16_t *, size_t,
                                                         HSD_TripleSumU64 *);

static atomic_uintptr_t hsd_jaccard_get_sums_ptr =
    ATOMIC_VAR_INIT((uintptr_t)jaccard_get_sums_resolver_trampoline);

hsd_status_t hsd_sim_jaccard_u16(const uint16_t *a, const uint16_t *b, size_t n, float *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 1.0f;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = NAN;
        return HSD_ERR_NULL_PTR;
    }

    hsd_jaccard_get_sums_func_t func = (hsd_jaccard_get_sums_func_t)atomic_load_explicit(
        &hsd_jaccard_get_sums_ptr, memory_order_acquire);

    HSD_TripleSumU64 sums = {0, 0, 0};
    hsd_status_t st = func(a, b, n, &sums);
    if (st != HSD_SUCCESS) {
        *result = NAN;
        return st;
    }
    return calculate_jaccard_similarity_from_sums_u64(sums.dot_product, sums.norm_a_sq,
                                                      sums.norm_b_sq, result);
}

static hsd_status_t jaccard_get_sums_resolver_trampoline(const uint16_t *a, const uint16_t *b,
                                                         size_t n, HSD_TripleSumU64 *sums) {
    hsd_jaccard_get_sums_func_t resolved = resolve_jaccard_get_sums_internal();

    uintptr_t expect = (uintptr_t)jaccard_get_sums_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_jaccard_get_sums_ptr, &expect, (uintptr_t)resolved,
                                            memory_order_release, memory_order_relaxed);
    hsd_jaccard_get_sums_func_t current_func = (hsd_jaccard_get_sums_func_t)atomic_load_explicit(
        &hsd_jaccard_get_sums_ptr, memory_order_acquire);

    return current_func(a, b, n, sums);
}

static hsd_jaccard_get_sums_func_t resolve_jaccard_get_sums_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();

    hsd_jaccard_get_sums_func_t chosen = jaccard_get_sums_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Jaccard U16: Forced backend %d", forced);
        bool supported = false;
        switch (forced) {
#if defined(__x86_64__) || defined(_M_X64)
            case HSD_BACKEND_AVX512BW:
            case HSD_BACKEND_AVX512DQ:
                if (hsd_cpu_has_avx512f() && hsd_cpu_has_avx512bw() && hsd_cpu_has_avx512dq()) {
                    chosen = jaccard_get_sums_avx512_internal;
                    reason = "AVX512 F+BW+DQ (Forced)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX2:
                if (hsd_cpu_has_avx2()) {
                    chosen = jaccard_get_sums_avx2_internal;
                    reason = "AVX2 (Forced)";
                    supported = true;
                }
                break;
#endif
#if defined(__aarch64__) || defined(__arm__)
            case HSD_BACKEND_NEON:
                if (hsd_cpu_has_neon()) {
                    chosen = jaccard_get_sums_neon_internal;
                    reason = "NEON (Forced)";
                    supported = true;
                }
                break;
#if defined(__ARM_FEATURE_SVE)
            case HSD_BACKEND_SVE:
                if (hsd_cpu_has_sve()) {
                    chosen = jaccard_get_sums_sve_internal;
                    reason = "SVE (Forced)";
                    supported = true;
                }
                break;
#endif
#endif
            case HSD_BACKEND_SCALAR:
                chosen = jaccard_get_sums_scalar_internal;
                reason = "Scalar (Forced)";
                supported = true;
                break;
            default:
                reason = "Scalar (Forced backend invalid)";
                chosen = jaccard_get_sums_scalar_internal;
                break;
        }

        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Forced backend %d not supported, falling back to Scalar", forced);
            chosen = jaccard_get_sums_scalar_internal;
            reason = "Scalar (Fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)

        if (hsd_cpu_has_avx512f() && hsd_cpu_has_avx512bw() && hsd_cpu_has_avx512dq()) {
            chosen = jaccard_get_sums_avx512_internal;
            reason = "AVX512 F+BW+DQ (Auto)";
        } else if (hsd_cpu_has_avx2()) {
            chosen = jaccard_get_sums_avx2_internal;
            reason = "AVX2 (Auto)";
        }
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)
        if (hsd_cpu_has_sve()) {
            chosen = jaccard_get_sums_sve_internal;
            reason = "SVE (Auto)";
        } else if (hsd_cpu_has_neon()) {
            chosen = jaccard_get_sums_neon_internal;
            reason = "NEON (Auto)";
        }
#else
        if (hsd_cpu_has_neon()) {
            chosen = jaccard_get_sums_neon_internal;
            reason = "NEON (Auto)";
        }
#endif
#endif
    }

    hsd_log("Dispatch: Resolved Jaccard U16 to: %s", reason);
    return chosen;
}
