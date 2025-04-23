#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif
#if defined(__aarch64__)
extern bool hsd_cpu_has_neon(void);
#if defined(__ARM_FEATURE_SVE)
extern bool hsd_cpu_has_sve(void);
#endif
#endif
#endif

typedef hsd_status_t (*hsd_hamming_u8_func_t)(const uint8_t *, const uint8_t *, size_t, uint64_t *);

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
static inline uint8_t hsd_internal_popcount8(uint8_t val) {
#if __has_builtin(__builtin_popcount)
    return (uint8_t)__builtin_popcount(val);
#else
    uint8_t count = 0;
    while (val) {
        val &= (val - 1);
        count++;
    }
    return count;
#endif
}

static hsd_status_t hamming_scalar_internal(const uint8_t *a, const uint8_t *b, size_t n,
                                            uint64_t *result) {
    hsd_log("Enter hamming_scalar_internal (n=%zu)", n);
    uint64_t total = 0;
    for (size_t i = 0; i < n; ++i) {
        total += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    }
    *result = total;
    return HSD_SUCCESS;
}

#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx512f,avx512vpopcntdq"))) static hsd_status_t
hamming_avx512_vpopcntdq_internal(const uint8_t *a, const uint8_t *b, size_t n, uint64_t *result) {
    hsd_log("Enter hamming_avx512_vpopcntdq_internal (n=%zu)", n);
    size_t i = 0;
    __m512i acc = _mm512_setzero_si512();
    for (; i + 64 <= n; i += 64) {
        __m512i va = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb = _mm512_loadu_si512((const __m512i *)(b + i));
        __m512i x = _mm512_xor_si512(va, vb);
        acc = _mm512_add_epi64(acc, _mm512_popcnt_epi64(x));
    }
    uint64_t sums[8];
    _mm512_storeu_si512((__m512i *)sums, acc);
    uint64_t total = 0;
    for (int j = 0; j < 8; ++j) total += sums[j];
    for (; i < n; ++i) total += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    *result = total;
    return HSD_SUCCESS;
}

__attribute__((target("avx2"))) static hsd_status_t hamming_avx2_pshufb_internal(const uint8_t *a,
                                                                                 const uint8_t *b,
                                                                                 size_t n,
                                                                                 uint64_t *result) {
    hsd_log("Enter hamming_avx2_pshufb_internal (n=%zu)", n);

    static const uint8_t popcount_table[32] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                                               0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

    const __m256i lookup = _mm256_loadu_si256((const __m256i *)popcount_table);
    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    size_t i = 0;
    __m256i acc = _mm256_setzero_si256();

    for (; i + 32 <= n; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        __m256i x = _mm256_xor_si256(va, vb);

        __m256i lo = _mm256_and_si256(x, low_mask);
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(x, 4), low_mask);

        __m256i pc_lo = _mm256_shuffle_epi8(lookup, lo);
        __m256i pc_hi = _mm256_shuffle_epi8(lookup, hi);

        __m256i pc = _mm256_add_epi8(pc_lo, pc_hi);
        acc = _mm256_add_epi64(acc, _mm256_sad_epu8(pc, _mm256_setzero_si256()));
    }

    uint64_t sums[4];
    _mm256_storeu_si256((__m256i *)sums, acc);
    uint64_t total = sums[0] + sums[1] + sums[2] + sums[3];

    for (; i < n; ++i) {
        total += hsd_internal_popcount8(a[i] ^ b[i]);
    }

    *result = total;
    return HSD_SUCCESS;
}

#endif

#if defined(__aarch64__) || defined(__arm__)
static hsd_status_t hamming_neon_internal(const uint8_t *a, const uint8_t *b, size_t n,
                                          uint64_t *result) {
    hsd_log("Enter hamming_neon_internal (n=%zu)", n);
    size_t i = 0;
    uint64x2_t acc = vdupq_n_u64(0);
    for (; i + 16 <= n; i += 16) {
        uint8x16_t va = vld1q_u8(a + i);
        uint8x16_t vb = vld1q_u8(b + i);
        uint8x16_t x = veorq_u8(va, vb);
        uint8x16_t pc = vcntq_u8(x);
        acc = vpadalq_u32(acc, vpaddlq_u16(vpaddlq_u8(pc)));
    }
#if defined(__aarch64__)
    uint64_t total = vaddvq_u64(acc);
#else
    uint64_t total = vgetq_lane_u64(acc, 0) + vgetq_lane_u64(acc, 1);
#endif
    for (; i < n; ++i) total += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    *result = total;
    return HSD_SUCCESS;
}

#if defined(__ARM_FEATURE_SVE)
__attribute__((target("+sve"))) static hsd_status_t hamming_sve_internal(const uint8_t *a,
                                                                         const uint8_t *b, size_t n,
                                                                         uint64_t *result) {
    hsd_log("Enter hamming_sve_internal (n=%zu)", n);
    int64_t i = 0;
    int64_t n_sve = (int64_t)n;
    uint64_t total_sum = 0;

    while (i < n_sve) {
        svbool_t pg_b8 = svwhilelt_b8((uint64_t)i, (uint64_t)n);

        svuint8_t va = svld1_u8(pg_b8, a + i);
        svuint8_t vb = svld1_u8(pg_b8, b + i);
        svuint8_t x = sveor_z(pg_b8, va, vb);
        svuint8_t pc8 = svcnt_u8_z(pg_b8, x);

        svuint16_t pc16_lo = svunpklo_u16(pc8);
        svuint16_t pc16_hi = svunpkhi_u16(pc8);

        svbool_t pg_b16 = svwhilelt_b16((uint64_t)i, (uint64_t)n);

        uint64_t sum16_lo = svaddv_u16(pg_b16, pc16_lo);
        uint64_t sum16_hi = svaddv_u16(pg_b16, pc16_hi);

        total_sum += sum16_lo + sum16_hi;

        i += svcntb();
    }

    *result = total_sum;
    return HSD_SUCCESS;
}
#endif
#endif

static hsd_hamming_u8_func_t resolve_hamming_u8_internal(void);
static hsd_status_t hamming_u8_resolver_trampoline(const uint8_t *, const uint8_t *, size_t,
                                                   uint64_t *);

static atomic_uintptr_t hsd_hamming_u8_ptr =
    ATOMIC_VAR_INIT((uintptr_t)hamming_u8_resolver_trampoline);

hsd_status_t hsd_dist_hamming_u8(const uint8_t *a, const uint8_t *b, size_t n, uint64_t *result) {
    if (result == NULL) return HSD_ERR_NULL_PTR;
    if (n == 0) {
        *result = 0;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        *result = UINT64_MAX;
        return HSD_ERR_NULL_PTR;
    }
    hsd_hamming_u8_func_t func =
        (hsd_hamming_u8_func_t)atomic_load_explicit(&hsd_hamming_u8_ptr, memory_order_acquire);
    return func(a, b, n, result);
}

static hsd_status_t hamming_u8_resolver_trampoline(const uint8_t *a, const uint8_t *b, size_t n,
                                                   uint64_t *result) {
    hsd_hamming_u8_func_t resolved = resolve_hamming_u8_internal();
    uintptr_t exp = (uintptr_t)hamming_u8_resolver_trampoline;
    atomic_compare_exchange_strong_explicit(&hsd_hamming_u8_ptr, &exp, (uintptr_t)resolved,
                                            memory_order_release, memory_order_relaxed);
    return resolved(a, b, n, result);
}

static hsd_hamming_u8_func_t resolve_hamming_u8_internal(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    hsd_hamming_u8_func_t chosen_func = hamming_scalar_internal;
    const char *reason = "Scalar (Default)";

    if (forced != HSD_BACKEND_AUTO) {
        hsd_log("Hamming U8: Forced backend %d", forced);
        bool supported = false;
        switch (forced) {
#if defined(__x86_64__) || defined(_M_X64)
            case HSD_BACKEND_AVX512VPOPCNTDQ:
                if (hsd_cpu_has_avx512f() && hsd_cpu_has_avx512vpopcntdq()) {
                    chosen_func = hamming_avx512_vpopcntdq_internal;
                    reason = "AVX512VPOPCNTDQ (Forced)";
                    supported = true;
                }
                break;
            case HSD_BACKEND_AVX2:
                if (hsd_cpu_has_avx2()) {
                    chosen_func = hamming_avx2_pshufb_internal;
                    reason = "AVX2 (Forced)";
                    supported = true;
                }
                break;
#endif
#if defined(__aarch64__) || defined(__arm__)
            case HSD_BACKEND_NEON:
                if (hsd_cpu_has_neon()) {
                    chosen_func = hamming_neon_internal;
                    reason = "NEON (Forced)";
                    supported = true;
                }
                break;
#if defined(__ARM_FEATURE_SVE)
            case HSD_BACKEND_SVE:
                if (hsd_cpu_has_sve()) {
                    chosen_func = hamming_sve_internal;
                    reason = "SVE (Forced)";
                    supported = true;
                }
                break;
#endif
#endif
            case HSD_BACKEND_SCALAR:
                chosen_func = hamming_scalar_internal;
                reason = "Scalar (Forced)";
                supported = true;
                break;
            default:
                reason = "Scalar (Forced backend invalid)";
                chosen_func = hamming_scalar_internal;
                break;
        }
        if (!(supported) && forced != HSD_BACKEND_SCALAR) {
            hsd_log("Forced backend %d not supported; falling back to Scalar.", forced);
            chosen_func = hamming_scalar_internal;
            reason = "Scalar (Fallback)";
        }
    } else {
        reason = "Scalar (Auto)";
#if defined(__x86_64__) || defined(_M_X64)
        if (hsd_cpu_has_avx512f() && hsd_cpu_has_avx512vpopcntdq()) {
            chosen_func = hamming_avx512_vpopcntdq_internal;
            reason = "AVX512VPOPCNTDQ (Auto)";
        } else if (hsd_cpu_has_avx2()) {
            chosen_func = hamming_avx2_pshufb_internal;
            reason = "AVX2 (Auto)";
        }
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__ARM_FEATURE_SVE)
        if (hsd_cpu_has_sve()) {
            chosen_func = hamming_sve_internal;
            reason = "SVE (Auto)";
        } else if (hsd_cpu_has_neon()) {
            chosen_func = hamming_neon_internal;
            reason = "NEON (Auto)";
        }
#else
        if (hsd_cpu_has_neon()) {
            chosen_func = hamming_neon_internal;
            reason = "NEON (Auto)";
        }
#endif
#endif
    }

    hsd_log("Dispatch: Resolved Hamming U8 to: %s", reason);
    return chosen_func;
}
