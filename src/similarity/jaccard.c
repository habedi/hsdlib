#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
// Declarations for functions defined in utils.c, used in resolver
extern bool hsd_cpu_has_avx512f(void);
extern bool hsd_cpu_has_avx512bw(void);
extern bool hsd_cpu_has_avx512dq(void);  // Assumes added in utils.c/hsdlib.h
extern bool hsd_cpu_has_avx2(void);
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
// Declarations for functions defined in utils.c, used in resolver
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
    double denominator = d_nAsq + d_nBsq - d_dot;
    double similarity;
    if (denominator < 1e-9) {
        similarity = 1.0;
    } else {
        similarity = d_dot / denominator;
    }
    if (similarity > 1.0) similarity = 1.0;
    if (similarity < 0.0) similarity = 0.0;
    *result = (float)similarity;
    if (isnan(*result) || isinf(*result)) {
        *result = NAN;
        return HSD_ERR_INVALID_INPUT;
    }
    return HSD_SUCCESS;
}

static hsd_status_t jaccard_get_sums_scalar_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                     HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_scalar_internal<u16> (n=%zu)", n);
    uint64_t dot_p = 0;
    uint64_t n_a_sq = 0;
    uint64_t n_b_sq = 0;
    for (size_t i = 0; i < n; ++i) {
        uint64_t val_a = (uint64_t)a[i];
        uint64_t val_b = (uint64_t)b[i];
        dot_p += val_a * val_b;
        n_a_sq += val_a * val_a;
        n_b_sq += val_b * val_b;
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
    __m256i nAsq_acc = _mm256_setzero_si256();
    __m256i nBsq_acc = _mm256_setzero_si256();
    for (; i + 16 <= n; i += 16) {
        __m256i va16 = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb16 = _mm256_loadu_si256((const __m256i *)(b + i));
        __m128i va16_lo = _mm256_castsi256_si128(va16);
        __m128i va16_hi = _mm256_extracti128_si256(va16, 1);
        __m128i vb16_lo = _mm256_castsi256_si128(vb16);
        __m128i vb16_hi = _mm256_extracti128_si256(vb16, 1);
        __m256i va32_lo = _mm256_cvtepu16_epi32(va16_lo);
        __m256i va32_hi = _mm256_cvtepu16_epi32(va16_hi);
        __m256i vb32_lo = _mm256_cvtepu16_epi32(vb16_lo);
        __m256i vb32_hi = _mm256_cvtepu16_epi32(vb16_hi);
        __m256i dot32_lo = _mm256_mullo_epi32(va32_lo, vb32_lo);
        __m256i dot32_hi = _mm256_mullo_epi32(va32_hi, vb32_hi);
        __m256i nAsq32_lo = _mm256_mullo_epi32(va32_lo, va32_lo);
        __m256i nAsq32_hi = _mm256_mullo_epi32(va32_hi, va32_hi);
        __m256i nBsq32_lo = _mm256_mullo_epi32(vb32_lo, vb32_lo);
        __m256i nBsq32_hi = _mm256_mullo_epi32(vb32_hi, vb32_hi);

        dot_acc =
            _mm256_add_epi64(dot_acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot32_lo, 0)));
        dot_acc =
            _mm256_add_epi64(dot_acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot32_lo, 1)));
        dot_acc =
            _mm256_add_epi64(dot_acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot32_hi, 0)));
        dot_acc =
            _mm256_add_epi64(dot_acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(dot32_hi, 1)));

        nAsq_acc = _mm256_add_epi64(nAsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nAsq32_lo, 0)));
        nAsq_acc = _mm256_add_epi64(nAsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nAsq32_lo, 1)));
        nAsq_acc = _mm256_add_epi64(nAsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nAsq32_hi, 0)));
        nAsq_acc = _mm256_add_epi64(nAsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nAsq32_hi, 1)));

        nBsq_acc = _mm256_add_epi64(nBsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nBsq32_lo, 0)));
        nBsq_acc = _mm256_add_epi64(nBsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nBsq32_lo, 1)));
        nBsq_acc = _mm256_add_epi64(nBsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nBsq32_hi, 0)));
        nBsq_acc = _mm256_add_epi64(nBsq_acc,
                                    _mm256_cvtepu32_epi64(_mm256_extracti128_si256(nBsq32_hi, 1)));
    }

    uint64_t dot_sums[4], nAsq_sums[4], nBsq_sums[4];
    _mm256_storeu_si256((__m256i *)dot_sums, dot_acc);
    _mm256_storeu_si256((__m256i *)nAsq_sums, nAsq_acc);
    _mm256_storeu_si256((__m256i *)nBsq_sums, nBsq_acc);
    uint64_t dot_p = dot_sums[0] + dot_sums[1] + dot_sums[2] + dot_sums[3];
    uint64_t n_a_sq = nAsq_sums[0] + nAsq_sums[1] + nAsq_sums[2] + nAsq_sums[3];
    uint64_t n_b_sq = nBsq_sums[0] + nBsq_sums[1] + nBsq_sums[2] + nBsq_sums[3];

    for (; i < n; ++i) {
        uint64_t va = (uint64_t)a[i];
        uint64_t vb = (uint64_t)b[i];
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
    __m512i nAsq_acc = _mm512_setzero_si512();
    __m512i nBsq_acc = _mm512_setzero_si512();
    for (; i + 32 <= n; i += 32) {
        __m512i va16 = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb16 = _mm512_loadu_si512((const __m512i *)(b + i));
        __m256i va16_lo = _mm512_extracti64x4_epi64(va16, 0);
        __m256i vb16_lo = _mm512_extracti64x4_epi64(vb16, 0);
        __m512i va32_lo = _mm512_cvtepu16_epi32(va16_lo);
        __m512i vb32_lo = _mm512_cvtepu16_epi32(vb16_lo);
        __m512i dot32_lo = _mm512_mullo_epi32(va32_lo, vb32_lo);
        __m512i nAsq32_lo = _mm512_mullo_epi32(va32_lo, va32_lo);
        __m512i nBsq32_lo = _mm512_mullo_epi32(vb32_lo, vb32_lo);

        __m256i va16_hi = _mm512_extracti64x4_epi64(va16, 1);
        __m256i vb16_hi = _mm512_extracti64x4_epi64(vb16, 1);
        __m512i va32_hi = _mm512_cvtepu16_epi32(va16_hi);
        __m512i vb32_hi = _mm512_cvtepu16_epi32(vb16_hi);
        __m512i dot32_hi = _mm512_mullo_epi32(va32_hi, vb32_hi);
        __m512i nAsq32_hi = _mm512_mullo_epi32(va32_hi, va32_hi);
        __m512i nBsq32_hi = _mm512_mullo_epi32(vb32_hi, vb32_hi);

        dot_acc = _mm512_add_epi64(dot_acc,
                                   _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot32_lo, 0)));
        dot_acc = _mm512_add_epi64(dot_acc,
                                   _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot32_lo, 1)));
        dot_acc = _mm512_add_epi64(dot_acc,
                                   _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot32_hi, 0)));
        dot_acc = _mm512_add_epi64(dot_acc,
                                   _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(dot32_hi, 1)));

        nAsq_acc = _mm512_add_epi64(nAsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nAsq32_lo, 0)));
        nAsq_acc = _mm512_add_epi64(nAsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nAsq32_lo, 1)));
        nAsq_acc = _mm512_add_epi64(nAsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nAsq32_hi, 0)));
        nAsq_acc = _mm512_add_epi64(nAsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nAsq32_hi, 1)));

        nBsq_acc = _mm512_add_epi64(nBsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nBsq32_lo, 0)));
        nBsq_acc = _mm512_add_epi64(nBsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nBsq32_lo, 1)));
        nBsq_acc = _mm512_add_epi64(nBsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nBsq32_hi, 0)));
        nBsq_acc = _mm512_add_epi64(nBsq_acc,
                                    _mm512_cvtepu32_epi64(_mm512_extracti32x8_epi32(nBsq32_hi, 1)));
    }

    uint64_t dot_hsum[8], nAsq_hsum[8], nBsq_hsum[8];
    _mm512_storeu_si512((__m512i *)dot_hsum, dot_acc);
    _mm512_storeu_si512((__m512i *)nAsq_hsum, nAsq_acc);
    _mm512_storeu_si512((__m512i *)nBsq_hsum, nBsq_acc);

    uint64_t dot_p = 0, n_a_sq = 0, n_b_sq = 0;
    for (int k = 0; k < 8; ++k) {
        dot_p += dot_hsum[k];
        n_a_sq += nAsq_hsum[k];
        n_b_sq += nBsq_hsum[k];
    }
    for (; i < n; ++i) {
        uint64_t va = (uint64_t)a[i];
        uint64_t vb = (uint64_t)b[i];
        dot_p += va * vb;
        n_a_sq += va * va;
        n_b_sq += vb * vb;
    }

    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    return HSD_SUCCESS;
}
#endif  // defined(__x86_64__) || defined(_M_X64)

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t jaccard_get_sums_neon_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                   HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_neon_internal<u16> (n=%zu)", n);
    size_t i = 0;
    uint64x2_t dot_acc = vdupq_n_u64(0);
    uint64x2_t nAsq_acc = vdupq_n_u64(0);
    uint64x2_t nBsq_acc = vdupq_n_u64(0);
    for (; i + 8 <= n; i += 8) {
        uint16x8_t va16 = vld1q_u16(a + i);
        uint16x8_t vb16 = vld1q_u16(b + i);

        uint32x4_t dot32_lo = vmull_u16(vget_low_u16(va16), vget_low_u16(vb16));
        uint32x4_t dot32_hi = vmull_u16(vget_high_u16(va16), vget_high_u16(vb16));

        uint32x4_t nAsq32_lo = vmull_u16(vget_low_u16(va16), vget_low_u16(va16));
        uint32x4_t nAsq32_hi = vmull_u16(vget_high_u16(va16), vget_high_u16(va16));

        uint32x4_t nBsq32_lo = vmull_u16(vget_low_u16(vb16), vget_low_u16(vb16));
        uint32x4_t nBsq32_hi = vmull_u16(vget_high_u16(vb16), vget_high_u16(vb16));

        dot_acc = vpadalq_u32(dot_acc, dot32_lo);
        dot_acc = vpadalq_u32(dot_acc, dot32_hi);
        nAsq_acc = vpadalq_u32(nAsq_acc, nAsq32_lo);
        nAsq_acc = vpadalq_u32(nAsq_acc, nAsq32_hi);
        nBsq_acc = vpadalq_u32(nBsq_acc, nBsq32_lo);
        nBsq_acc = vpadalq_u32(nBsq_acc, nBsq32_hi);
    }
#if defined(__aarch64__)
    uint64_t dot_p = vaddvq_u64(dot_acc);
    uint64_t n_a_sq = vaddvq_u64(nAsq_acc);
    uint64_t n_b_sq = vaddvq_u64(nBsq_acc);
#else
    uint64_t dot_p = vgetq_lane_u64(dot_acc, 0) + vgetq_lane_u64(dot_acc, 1);
    uint64_t n_a_sq = vgetq_lane_u64(nAsq_acc, 0) + vgetq_lane_u64(nAsq_acc, 1);
    uint64_t n_b_sq = vgetq_lane_u64(nBsq_acc, 0) + vgetq_lane_u64(nBsq_acc, 1);
#endif
    for (; i < n; ++i) {
        uint64_t va = (uint64_t)a[i];
        uint64_t vb = (uint64_t)b[i];
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
    int64_t n_sve = (int64_t)n;  // Use for loop comparison
    svbool_t pg;
    svuint64_t dot_acc = svdup_n_u64(0);
    svuint64_t nAsq_acc = svdup_n_u64(0);
    svuint64_t nBsq_acc = svdup_n_u64(0);
    uint64_t step = svcnth();  // For explicit step check if needed, but svwhilelt handles it

    while (i < n_sve) {
        // FIX: Cast loop counter/bound to uint64_t for predicate generation
        pg = svwhilelt_b16((uint64_t)i, (uint64_t)n);

        svuint16_t va16 = svld1_u16(pg, a + i);
        svuint16_t vb16 = svld1_u16(pg, b + i);

        svuint32_t va32_lo = svunpklo_u32(va16);
        svuint32_t va32_hi = svunpkhi_u32(va16);
        svuint32_t vb32_lo = svunpklo_u32(vb16);
        svuint32_t vb32_hi = svunpkhi_u32(vb16);

        svuint32_t dot32_lo = svmul_u32_z(pg, va32_lo, vb32_lo);   // Zeroing OK
        svuint32_t dot32_hi = svmul_u32_z(pg, va32_hi, vb32_hi);   // Zeroing OK
        svuint32_t nAsq32_lo = svmul_u32_z(pg, va32_lo, va32_lo);  // Zeroing OK
        svuint32_t nAsq32_hi = svmul_u32_z(pg, va32_hi, va32_hi);  // Zeroing OK
        svuint32_t nBsq32_lo = svmul_u32_z(pg, vb32_lo, vb32_lo);  // Zeroing OK
        svuint32_t nBsq32_hi = svmul_u32_z(pg, vb32_hi, vb32_hi);  // Zeroing OK

        // Accumulation is NOT predicated, operates on zeroed inputs - OK
        dot_acc = svaddwb_u64(dot_acc, dot32_lo);
        dot_acc = svaddwt_u64(dot_acc, dot32_hi);
        nAsq_acc = svaddwb_u64(nAsq_acc, nAsq32_lo);
        nAsq_acc = svaddwt_u64(nAsq_acc, nAsq32_hi);
        nBsq_acc = svaddwb_u64(nBsq_acc, nBsq32_lo);
        nBsq_acc = svaddwt_u64(nBsq_acc, nBsq32_hi);

        i += svcnth();  // Increment by number of uint16 elements processed
    }

    sums->dot_product = svaddv_u64(svptrue_b64(), dot_acc);
    sums->norm_a_sq = svaddv_u64(svptrue_b64(), nAsq_acc);
    sums->norm_b_sq = svaddv_u64(svptrue_b64(), nBsq_acc);
    return HSD_SUCCESS;
}
#endif  // __ARM_FEATURE_SVE
#endif  // __aarch64__ || __arm__

static hsd_jaccard_get_sums_func_t resolve_jaccard_get_sums_internal(void);
static hsd_status_t jaccard_get_sums_resolver_trampoline(const uint16_t *a, const uint16_t *b,
                                                         size_t n, HSD_TripleSumU64 *sums);

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
    hsd_status_t status = func(a, b, n, &sums);
    if (status != HSD_SUCCESS) {
        *result = NAN;
        return status;
    }

    return calculate_jaccard_similarity_from_sums_u64(sums.dot_product, sums.norm_a_sq,
                                                      sums.norm_b_sq, result);
}

static hsd_status_t jaccard_get_sums_resolver_trampoline(const uint16_t *a, const uint16_t *b,
                                                         size_t n, HSD_TripleSumU64 *sums) {
    hsd_jaccard_get_sums_func_t resolved = resolve_jaccard_get_sums_internal();
    uintptr_t expected = (uintptr_t)jaccard_get_sums_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_jaccard_get_sums_ptr, &expected,
                                            (uintptr_t)resolved, memory_order_release,
                                            memory_order_relaxed);
    return resolved(a, b, n, sums);
}

static hsd_jaccard_get_sums_func_t resolve_jaccard_get_sums_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_jaccard_get_sums_func_t chosen_func = jaccard_get_sums_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Jaccard U16: Forced backend %d", forced);
        bool supported = false;
#if defined(__x86_64__) || defined(_M_X64)
        if (forced == HSD_BACKEND_AVX512BW && hsd_cpu_has_avx512f() && hsd_cpu_has_avx512bw() &&
            hsd_cpu_has_avx512dq()) {
            chosen_func = jaccard_get_sums_avx512_internal;
            reason = "AVX512 F+BW+DQ (Forced)";
            supported = true;
        } else if (forced == HSD_BACKEND_AVX2 && hsd_cpu_has_avx2()) {
            chosen_func = jaccard_get_sums_avx2_internal;
            reason = "AVX2 (Forced)";
            supported = true;
        }
#endif
        if (!supported && forced == HSD_BACKEND_SCALAR) {
            chosen_func = jaccard_get_sums_scalar_internal;
            reason = "Scalar (Forced)";
            supported = true;
        }
        if (!supported && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Forced backend %d not supported, falling back", forced);
            chosen_func = jaccard_get_sums_scalar_internal;
            reason = "Scalar (Fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f() && hsd_cpu_has_avx512bw() && hsd_cpu_has_avx512dq()) {
            chosen_func = jaccard_get_sums_avx512_internal;
            reason = "AVX512 F+BW+DQ (Auto)";
        } else if (hsd_cpu_has_avx2()) {
            chosen_func = jaccard_get_sums_avx2_internal;
            reason = "AVX2 (Auto)";
        }
#elif defined(__aarch64__) || defined(__arm__)
        if (hsd_cpu_has_neon()) {
            chosen_func = jaccard_get_sums_neon_internal;
            reason = "NEON (Auto)";
        }
#endif
    }

    hsd_log("Dispatch: %s", reason);
    return chosen_func;
}
