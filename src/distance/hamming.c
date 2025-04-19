#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__AVX2__) || defined(__AVX512BW__) || defined(__AVX512VPOPCNTDQ__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif

#include "hsdlib.h"

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

static inline uint8_t hsd_internal_popcount8(uint8_t val) {
#if __has_builtin(__builtin_popcount)
    return (uint8_t)__builtin_popcount(val);
#else
    uint8_t count = 0;
    while (val > 0) {
        val &= (val - 1);
        count++;
    }
    return count;
#endif
}

static inline hsd_status_t hamming_scalar_internal(const uint8_t *a, const uint8_t *b, size_t n,
                                                   uint64_t *result) {
    hsd_log("Enter hamming_scalar_internal (n=%zu)", n);
    uint64_t total_diff_bits = 0;
    for (size_t i = 0; i < n; ++i) {
        total_diff_bits += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    }
    *result = total_diff_bits;
    hsd_log("Exit hamming_scalar_internal");
    return HSD_SUCCESS;
}

#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)
static inline hsd_status_t hamming_avx512_vpocntdq_internal(const uint8_t *a, const uint8_t *b,
                                                            size_t n, uint64_t *result) {
    hsd_log("Enter hamming_avx512_vpocntdq_internal (n=%zu)", n);
    size_t i = 0;
    uint64_t total_diff_bits = 0;
    __m512i popcnt_acc = _mm512_setzero_si512();

    for (; i + 64 <= n; i += 64) {
        __m512i va = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb = _mm512_loadu_si512((const __m512i *)(b + i));
        __m512i xor_res = _mm512_xor_si512(va, vb);
        __m512i popcnt64 = _mm512_popcnt_epi64(xor_res);
        popcnt_acc = _mm512_add_epi64(popcnt_acc, popcnt64);
    }
    total_diff_bits = _mm512_reduce_add_epi64(popcnt_acc);

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    }
    *result = total_diff_bits;
    hsd_log("Exit hamming_avx512_vpocntdq_internal");
    return HSD_SUCCESS;
}

#elif defined(__AVX2__)

static const uint8_t popcount_lookup_table[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static inline hsd_status_t hamming_avx2_pshufb_internal(const uint8_t *a, const uint8_t *b,
                                                        size_t n, uint64_t *result) {
    hsd_log("Enter hamming_avx2_pshufb_internal (n=%zu)", n);
    size_t i = 0;
    __m256i total_popcnt_sad = _mm256_setzero_si256();

    const __m128i popcount_lut_128 = _mm_loadu_si128((const __m128i *)popcount_lookup_table);
    const __m256i low_mask = _mm256_set1_epi8(0x0F);

    for (; i + 32 <= n; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        __m256i xor_res = _mm256_xor_si256(va, vb);

        __m256i low_nibbles = _mm256_and_si256(xor_res, low_mask);
        __m256i high_nibbles = _mm256_and_si256(_mm256_srli_epi16(xor_res, 4), low_mask);

        __m128i low_nib_lo = _mm256_castsi256_si128(low_nibbles);
        __m128i low_nib_hi = _mm256_extracti128_si256(low_nibbles, 1);
        __m128i high_nib_lo = _mm256_castsi256_si128(high_nibbles);
        __m128i high_nib_hi = _mm256_extracti128_si256(high_nibbles, 1);

        __m128i popcnt_low_lo = _mm_shuffle_epi8(popcount_lut_128, low_nib_lo);
        __m128i popcnt_low_hi = _mm_shuffle_epi8(popcount_lut_128, low_nib_hi);
        __m128i popcnt_high_lo = _mm_shuffle_epi8(popcount_lut_128, high_nib_lo);
        __m128i popcnt_high_hi = _mm_shuffle_epi8(popcount_lut_128, high_nib_hi);

        __m256i popcnt_low = _mm256_set_m128i(popcnt_low_hi, popcnt_low_lo);
        __m256i popcnt_high = _mm256_set_m128i(popcnt_high_hi, popcnt_high_lo);

        __m256i byte_popcnt = _mm256_add_epi8(popcnt_low, popcnt_high);

        total_popcnt_sad = _mm256_add_epi64(total_popcnt_sad,
                                            _mm256_sad_epu8(byte_popcnt, _mm256_setzero_si256()));
    }

    uint64_t sums[4];
    _mm256_storeu_si256((__m256i *)sums, total_popcnt_sad);
    uint64_t total_diff_bits = sums[0] + sums[2];

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    }
    *result = total_diff_bits;
    hsd_log("Exit hamming_avx2_pshufb_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t hamming_neon_internal(const uint8_t *a, const uint8_t *b, size_t n,
                                                 uint64_t *result) {
    hsd_log("Enter hamming_neon_internal (n=%zu)", n);
    size_t i = 0;
    uint64x2_t total_acc = vdupq_n_u64(0);

    for (; i + 16 <= n; i += 16) {
        uint8x16_t va = vld1q_u8(a + i);
        uint8x16_t vb = vld1q_u8(b + i);
        uint8x16_t xor_res = veorq_u8(va, vb);
        uint8x16_t popcnt_bytes = vcntq_u8(xor_res);

        uint16x8_t pair_sum16 = vpaddlq_u8(popcnt_bytes);
        uint32x4_t quad_sum32 = vpaddlq_u16(pair_sum16);
        uint64x2_t quad_sum64 = vpaddlq_u32(quad_sum32);
        total_acc = vaddq_u64(total_acc, quad_sum64);
    }
#if defined(__aarch64__)
    uint64_t total_diff_bits = vaddvq_u64(total_acc);
#else
    uint64_t total_diff_bits = vgetq_lane_u64(total_acc, 0) + vgetq_lane_u64(total_acc, 1);
#endif

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)hsd_internal_popcount8(a[i] ^ b[i]);
    }
    *result = total_diff_bits;
    hsd_log("Exit hamming_neon_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t hamming_sve_internal(const uint8_t *a, const uint8_t *b, size_t n,
                                                uint64_t *result) {
    hsd_log("Enter hamming_sve_internal (n=%zu)", n);
    uint64_t total_diff_bits = 0;
    int64_t i = 0;
    svbool_t pg;
    svuint64_t total_acc = svdup_n_u64(0);

    do {
        pg = svwhilelt_b8((uint64_t)i, (uint64_t)n);
        svuint8_t va = svld1_u8(pg, a + i);
        svuint8_t vb = svld1_u8(pg, b + i);
        svuint8_t xor_res = sveor_z(pg, va, vb);
        svuint8_t popcnt_bytes = svcnt_u8_z(pg, xor_res);

        total_acc = svadd_u64_z(pg, total_acc, svuaddv_u8(pg, popcnt_bytes));

        i += svcntb();
    } while (svptest_any(svptrue_b8(), pg));

    total_diff_bits = svaddv_u64(svptrue_b64(), total_acc);

    *result = total_diff_bits;
    hsd_log("Exit hamming_sve_internal");
    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_dist_hamming_u8(const uint8_t *a, const uint8_t *b, size_t n, uint64_t *result) {
    hsd_log("Enter hsd_dist_hamming_u8 (n=%zu)", n);

    if (result == NULL) {
        hsd_log("Result pointer is NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        hsd_log("n is 0, Hamming distance is 0.");
        *result = 0;
        return HSD_SUCCESS;
    }
    if (a == NULL || b == NULL) {
        hsd_log("Input array pointers are NULL for non-zero n!");
        *result = UINT64_MAX;  // Indicate error? Hamming is non-negative.
        return HSD_ERR_NULL_PTR;
    }

    hsd_status_t status = HSD_FAILURE;

    hsd_log("Using CPU backend...");

#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)
    hsd_log("CPU Path: AVX512 (VPOPCNTDQ)");
    status = hamming_avx512_vpocntdq_internal(a, b, n, result);
#elif defined(__AVX2__)
    hsd_log("CPU Path: AVX2 (PSHUFB)");
    status = hamming_avx2_pshufb_internal(a, b, n, result);
#elif defined(__ARM_FEATURE_SVE)
    hsd_log("CPU Path: SVE");
    status = hamming_sve_internal(a, b, n, result);
#elif defined(__ARM_NEON)
    hsd_log("CPU Path: NEON");
    status = hamming_neon_internal(a, b, n, result);
#else
    hsd_log("CPU Path: Scalar");
    status = hamming_scalar_internal(a, b, n, result);
#endif

    if (status != HSD_SUCCESS) {
        hsd_log("CPU backend failed (status=%d).", status);
    } else {
        hsd_log("CPU backend succeeded. Hamming distance: %lu", *result);
    }

    hsd_log("Exit hsd_dist_hamming_u8 (final status=%d)", status);
    return status;
}
