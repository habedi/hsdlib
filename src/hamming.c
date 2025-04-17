#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__AVX2__) || defined(__AVX512BW__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif

#include "hsdlib.h"

static inline hsd_status_t hamming_scalar_internal(const int8_t *a, const int8_t *b, size_t n,
                                                   uint64_t *result_u64) {
    hsd_log("Enter hamming_scalar_internal (n=%zu)", n);
    uint64_t total_diff_bits = 0;
    for (size_t i = 0; i < n; ++i) {
        total_diff_bits += (uint64_t)((uint8_t)a[i] ^ (uint8_t)b[i]);
    }
    *result_u64 = total_diff_bits;
    hsd_log("Exit hamming_scalar_internal");
    return HSD_SUCCESS;
}

#if defined(__AVX2__)
static inline hsd_status_t hamming_avx2_internal(const int8_t *a, const int8_t *b, size_t n,
                                                 uint64_t *result_u64) {
    hsd_log("Enter hamming_avx2_internal (n=%zu)", n);
    size_t i = 0;
    uint64_t total_diff_bits = 0;
    __m256i sum_acc_vec = _mm256_setzero_si256();
    const __m256i zero = _mm256_setzero_si256();

    for (; i + 32 <= n; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        __m256i xor_res = _mm256_xor_si256(va, vb);
        __m256i sad = _mm256_sad_epu8(xor_res, zero);
        sum_acc_vec = _mm256_add_epi64(sum_acc_vec, sad);
    }

    uint64_t sums[4];
    _mm256_storeu_si256((__m256i *)sums, sum_acc_vec);
    total_diff_bits += sums[0] + sums[2];

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)((uint8_t)a[i] ^ (uint8_t)b[i]);
    }
    *result_u64 = total_diff_bits;
    hsd_log("Exit hamming_avx2_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__AVX512BW__) && defined(__AVX512F__)
static inline hsd_status_t hamming_avx512_internal(const int8_t *a, const int8_t *b, size_t n,
                                                   uint64_t *result_u64) {
    hsd_log("Enter hamming_avx512_internal (n=%zu)", n);
    size_t i = 0;
    uint64_t total_diff_bits = 0;
    __m512i sum_acc_vec = _mm512_setzero_si512();
    const __m512i zero = _mm512_setzero_si512();

    for (; i + 64 <= n; i += 64) {
        __m512i va = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb = _mm512_loadu_si512((const __m512i *)(b + i));
        __m512i xor_res = _mm512_xor_si512(va, vb);
        __m512i sad = _mm512_sad_epu8(xor_res, zero);
        sum_acc_vec = _mm512_add_epi64(sum_acc_vec, sad);
    }
    total_diff_bits += _mm512_reduce_add_epi64(sum_acc_vec);

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)((uint8_t)a[i] ^ (uint8_t)b[i]);
    }
    *result_u64 = total_diff_bits;
    hsd_log("Exit hamming_avx512_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t hamming_neon_internal(const int8_t *a, const int8_t *b, size_t n,
                                                 uint64_t *result_u64) {
    hsd_log("Enter hamming_neon_internal (n=%zu)", n);
    size_t i = 0;
    uint64_t total_diff_bits = 0;
    uint64x2_t sum_acc_vec = vdupq_n_u64(0);

    for (; i + 16 <= n; i += 16) {
        uint8x16_t va = vld1q_u8((const uint8_t *)a + i);
        uint8x16_t vb = vld1q_u8((const uint8_t *)b + i);
        uint8x16_t xor_res = veorq_u8(va, vb);
        uint16x8_t sum16 = vpaddlq_u8(xor_res);
        uint32x4_t sum32 = vpaddlq_u16(sum16);
        uint64x2_t sum64 = vpaddlq_u32(sum32);
        sum_acc_vec = vaddq_u64(sum_acc_vec, sum64);
    }
    total_diff_bits += vgetq_lane_u64(sum_acc_vec, 0) + vgetq_lane_u64(sum_acc_vec, 1);

    for (; i < n; ++i) {
        total_diff_bits += (uint64_t)((uint8_t)a[i] ^ (uint8_t)b[i]);
    }
    *result_u64 = total_diff_bits;
    hsd_log("Exit hamming_neon_internal");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t hamming_sve_internal(const int8_t *a, const int8_t *b, size_t n,
                                                uint64_t *result_u64) {
    hsd_log("Enter hamming_sve_internal (n=%zu)", n);
    uint64_t total_diff_bits = 0;
    int64_t i = 0;
    svbool_t pg;

    do {
        pg = svwhilelt_b8((uint64_t)i, (uint64_t)n);
        svuint8_t va = svld1_u8(pg, (const uint8_t *)a + i);
        svuint8_t vb = svld1_u8(pg, (const uint8_t *)b + i);
        svuint8_t xor_res = sveor_z(pg, va, vb);

        total_diff_bits += (uint64_t)svaddv_u8(pg, xor_res);
        i += svcntb();
    } while (svptest_any(svptrue_b8(), pg));

    *result_u64 = total_diff_bits;
    hsd_log("Exit hamming_sve_internal");
    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_hamming_i8(const int8_t *a, const int8_t *b, size_t n, float *result) {
    hsd_log("Enter hsd_hamming_i8 (n=%zu, backend: %s)", n, hsd_get_backend());

    if (a == NULL || b == NULL || result == NULL) {
        hsd_log("Input pointers are NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        hsd_log("n is 0, Hamming distance is 0.");
        *result = 0.0f;
        return HSD_SUCCESS;
    }

    hsd_status_t status = HSD_FAILURE;
    uint64_t result_u64 = 0;

    hsd_log("Using CPU backend: %s", hsd_get_backend());

#if defined(__AVX512BW__) && defined(__AVX512F__)
    status = hamming_avx512_internal(a, b, n, &result_u64);
#elif defined(__AVX2__)
    status = hamming_avx2_internal(a, b, n, &result_u64);
#elif defined(__ARM_FEATURE_SVE)
    status = hamming_sve_internal(a, b, n, &result_u64);
#elif defined(__ARM_NEON)
    status = hamming_neon_internal(a, b, n, &result_u64);
#else
    status = hamming_scalar_internal(a, b, n, &result_u64);
#endif

    if (status != HSD_SUCCESS) {
        hsd_log("SIMD implementation failed, falling back to scalar");
        status = hamming_scalar_internal(a, b, n, &result_u64);
    }

    if (status == HSD_SUCCESS) {
        *result = (float)result_u64;
        hsd_log("Hamming distance: %f (count: %lu)", *result, result_u64);
    }

    hsd_log("Exit hsd_hamming_i8 (final status=%d)", status);
    return status;
}
