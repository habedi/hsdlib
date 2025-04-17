#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__AVX2__) || (defined(__AVX512BW__) && defined(__AVX512F__))
#include <immintrin.h>
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif

#include "hsdlib.h"

typedef struct {
    uint64_t dot_product;
    uint64_t norm_a_sq;
    uint64_t norm_b_sq;
} HSD_TripleSumU64;

static inline hsd_status_t calculate_jaccard_distance_from_sums_u64(uint64_t dot, uint64_t nAsq,
                                                                    uint64_t nBsq, float *result) {
    if (nAsq == 0 && nBsq == 0) {
        hsd_log("Input Check: Both vectors are zero.");
        *result = 0.0f;
        return HSD_SUCCESS;
    }

    double d_dot = (double)dot;
    double d_nAsq = (double)nAsq;
    double d_nBsq = (double)nBsq;
    double denominator = d_nAsq + d_nBsq - d_dot;
    double similarity;

    if (denominator < 1e-9) {
        hsd_log("Denominator Check: Denominator %.8e is near zero", denominator);
        similarity = 1.0;
    } else {
        similarity = d_dot / denominator;
    }

    if (similarity > 1.0) similarity = 1.0;
    if (similarity < 0.0) similarity = 0.0;

    *result = 1.0f - (float)similarity;

    if (isnan(*result) || isinf(*result)) {
        hsd_log("Final Result Check: Result is NaN or Inf (value: %.8e)", *result);
        return HSD_FAILURE;
    }
    return HSD_SUCCESS;
}

static inline hsd_status_t jaccard_scalar_internal(const uint16_t *a, const uint16_t *b, size_t n,
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
    hsd_log("Exit jaccard_scalar_internal<u16>");
    return HSD_SUCCESS;
}

#if defined(__AVX2__)
static inline hsd_status_t jaccard_avx2_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                 HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_avx2_internal<u16> (n=%zu)", n);
    size_t i = 0;
    __m256i dot_acc = _mm256_setzero_si256();
    __m256i nAsq_acc = _mm256_setzero_si256();
    __m256i nBsq_acc = _mm256_setzero_si256();

    for (; i + 16 <= n; i += 16) {
        __m256i va16 = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb16 = _mm256_loadu_si256((const __m256i *)(b + i));
        __m256i va32_lo = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(va16, 0));
        __m256i va32_hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(va16, 1));
        __m256i vb32_lo = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(vb16, 0));
        __m256i vb32_hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(vb16, 1));
        __m256i dot32_lo = _mm256_mullo_epi32(va32_lo, vb32_lo);
        __m256i dot32_hi = _mm256_mullo_epi32(va32_hi, vb32_hi);
        __m256i nAsq32_lo = _mm256_mullo_epi32(va32_lo, va32_lo);
        __m256i nAsq32_hi = _mm256_mullo_epi32(va32_hi, va32_hi);
        __m256i nBsq32_lo = _mm256_mullo_epi32(vb32_lo, vb32_lo);
        __m256i nBsq32_hi = _mm256_mullo_epi32(vb32_hi, vb32_hi);
        dot_acc = _mm256_add_epi32(dot_acc, dot32_lo);
        dot_acc = _mm256_add_epi32(dot_acc, dot32_hi);
        nAsq_acc = _mm256_add_epi32(nAsq_acc, nAsq32_lo);
        nAsq_acc = _mm256_add_epi32(nAsq_acc, nAsq32_hi);
        nBsq_acc = _mm256_add_epi32(nBsq_acc, nBsq32_lo);
        nBsq_acc = _mm256_add_epi32(nBsq_acc, nBsq32_hi);
    }
    uint32_t dot_sums[8], nAsq_sums[8], nBsq_sums[8];
    _mm256_storeu_si256((__m256i *)dot_sums, dot_acc);
    _mm256_storeu_si256((__m256i *)nAsq_sums, nAsq_acc);
    _mm256_storeu_si256((__m256i *)nBsq_sums, nBsq_acc);
    uint64_t dot_p = 0;
    uint64_t n_a_sq = 0;
    uint64_t n_b_sq = 0;
    for (int k = 0; k < 8; ++k) {
        dot_p += dot_sums[k];
        n_a_sq += nAsq_sums[k];
        n_b_sq += nBsq_sums[k];
    }
    for (; i < n; ++i) {
        uint64_t val_a = (uint64_t)a[i];
        uint64_t val_b = (uint64_t)b[i];
        dot_p += val_a * val_b;
        n_a_sq += val_a * val_a;
        n_b_sq += val_b * val_b;
    }
    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    hsd_log("Exit jaccard_avx2_internal<u16>");
    return HSD_SUCCESS;
}
#endif

#if defined(__AVX512BW__) && defined(__AVX512F__)
static inline hsd_status_t jaccard_avx512_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                   HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_avx512_internal<u16> (n=%zu)", n);
    size_t i = 0;
    __m512i dot_acc = _mm512_setzero_si512();
    __m512i nAsq_acc = _mm512_setzero_si512();
    __m512i nBsq_acc = _mm512_setzero_si512();

    for (; i + 32 <= n; i += 32) {
        __m512i va16 = _mm512_loadu_si512((const __m512i *)(a + i));
        __m512i vb16 = _mm512_loadu_si512((const __m512i *)(b + i));
        __m512i va32_lo = _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(va16, 0));
        __m512i va32_hi = _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(va16, 1));
        __m512i vb32_lo = _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(vb16, 0));
        __m512i vb32_hi = _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(vb16, 1));
        __m512i dot32_lo = _mm512_mullo_epi32(va32_lo, vb32_lo);
        __m512i dot32_hi = _mm512_mullo_epi32(va32_hi, vb32_hi);
        __m512i nAsq32_lo = _mm512_mullo_epi32(va32_lo, va32_lo);
        __m512i nAsq32_hi = _mm512_mullo_epi32(va32_hi, va32_hi);
        __m512i nBsq32_lo = _mm512_mullo_epi32(vb32_lo, vb32_lo);
        __m512i nBsq32_hi = _mm512_mullo_epi32(vb32_hi, vb32_hi);
        dot_acc = _mm512_add_epi32(dot_acc, dot32_lo);
        dot_acc = _mm512_add_epi32(dot_acc, dot32_hi);
        nAsq_acc = _mm512_add_epi32(nAsq_acc, nAsq32_lo);
        nAsq_acc = _mm512_add_epi32(nAsq_acc, nAsq32_hi);
        nBsq_acc = _mm512_add_epi32(nBsq_acc, nBsq32_lo);
        nBsq_acc = _mm512_add_epi32(nBsq_acc, nBsq32_hi);
    }
    sums->dot_product = (uint64_t)_mm512_reduce_add_epi32(dot_acc);
    sums->norm_a_sq = (uint64_t)_mm512_reduce_add_epi32(nAsq_acc);
    sums->norm_b_sq = (uint64_t)_mm512_reduce_add_epi32(nBsq_acc);

    for (; i < n; ++i) {
        uint64_t val_a = (uint64_t)a[i];
        uint64_t val_b = (uint64_t)b[i];
        sums->dot_product += val_a * val_b;
        sums->norm_a_sq += val_a * val_a;
        sums->norm_b_sq += val_b * val_b;
    }
    hsd_log("Exit jaccard_avx512_internal<u16>");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_NEON)
static inline hsd_status_t jaccard_neon_internal(const uint16_t *a, const uint16_t *b, size_t n,
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
    uint64_t dot_p = vgetq_lane_u64(dot_acc, 0) + vgetq_lane_u64(dot_acc, 1);
    uint64_t n_a_sq = vgetq_lane_u64(nAsq_acc, 0) + vgetq_lane_u64(nAsq_acc, 1);
    uint64_t n_b_sq = vgetq_lane_u64(nBsq_acc, 0) + vgetq_lane_u64(nBsq_acc, 1);

    for (; i < n; ++i) {
        uint64_t val_a = (uint64_t)a[i];
        uint64_t val_b = (uint64_t)b[i];
        dot_p += val_a * val_b;
        n_a_sq += val_a * val_a;
        n_b_sq += val_b * val_b;
    }
    sums->dot_product = dot_p;
    sums->norm_a_sq = n_a_sq;
    sums->norm_b_sq = n_b_sq;
    hsd_log("Exit jaccard_neon_internal<u16>");
    return HSD_SUCCESS;
}
#endif

#if defined(__ARM_FEATURE_SVE)
static inline hsd_status_t jaccard_sve_internal(const uint16_t *a, const uint16_t *b, size_t n,
                                                HSD_TripleSumU64 *sums) {
    hsd_log("Enter jaccard_sve_internal<u16> (n=%zu)", n);
    int64_t i = 0;
    svbool_t pg;
    svuint64_t dot_acc = svdup_n_u64(0);
    svuint64_t nAsq_acc = svdup_n_u64(0);
    svuint64_t nBsq_acc = svdup_n_u64(0);

    do {
        pg = svwhilelt_b16((uint64_t)i, (uint64_t)n);
        svuint16_t va16 = svld1_u16(pg, a + i);
        svuint16_t vb16 = svld1_u16(pg, b + i);

        svuint32_t va32_b = svuxtlb_u32(pg, va16);
        svuint32_t va32_t = svuxtlt_u32(pg, va16);
        svuint32_t vb32_b = svuxtlb_u32(pg, vb16);
        svuint32_t vb32_t = svuxtlt_u32(pg, vb16);

        svuint32_t dot32_b = svmul_u32_z(pg, va32_b, vb32_b);
        svuint32_t dot32_t = svmul_u32_z(pg, va32_t, vb32_t);
        svuint32_t nAsq32_b = svmul_u32_z(pg, va32_b, va32_b);
        svuint32_t nAsq32_t = svmul_u32_z(pg, va32_t, va32_t);
        svuint32_t nBsq32_b = svmul_u32_z(pg, vb32_b, vb32_b);
        svuint32_t nBsq32_t = svmul_u32_z(pg, vb32_t, vb32_t);

        dot_acc = svadd_u64_z(pg, dot_acc, svuxtlb_u64(pg, dot32_b));
        dot_acc = svadd_u64_z(pg, dot_acc, svuxtlt_u64(pg, dot32_t));
        nAsq_acc = svadd_u64_z(pg, nAsq_acc, svuxtlb_u64(pg, nAsq32_b));
        nAsq_acc = svadd_u64_z(pg, nAsq_acc, svuxtlt_u64(pg, nAsq32_t));
        nBsq_acc = svadd_u64_z(pg, nBsq_acc, svuxtlb_u64(pg, nBsq32_b));
        nBsq_acc = svadd_u64_z(pg, nBsq_acc, svuxtlt_u64(pg, nBsq32_t));

        i += svcnth();
    } while (svptest_any(svptrue_b16(), pg));

    sums->dot_product = svaddv_u64(svptrue_b64(), dot_acc);
    sums->norm_a_sq = svaddv_u64(svptrue_b64(), nAsq_acc);
    sums->norm_b_sq = svaddv_u64(svptrue_b64(), nBsq_acc);
    hsd_log("Exit jaccard_sve_internal<u16>");
    return HSD_SUCCESS;
}
#endif

hsd_status_t hsd_jaccard_u16(const uint16_t *a, const uint16_t *b, size_t n, float *result) {
    hsd_log("Enter hsd_jaccard_u16 (n=%zu)", n);

    if (a == NULL || b == NULL || result == NULL) {
        hsd_log("Input pointers are NULL!");
        return HSD_ERR_NULL_PTR;
    }
    if (n == 0) {
        hsd_log("n is 0, Jaccard distance is 0.");
        *result = 0.0f;
        return HSD_SUCCESS;
    }

    hsd_status_t status = HSD_FAILURE;
    HSD_TripleSumU64 sums = {0, 0, 0};

    hsd_log("Using CPU backend...");
#if defined(__AVX512BW__) && defined(__AVX512F__)
    hsd_log("CPU Path: AVX512BW");
    status = jaccard_avx512_internal(a, b, n, &sums);
#elif defined(__AVX2__)
    hsd_log("CPU Path: AVX2");
    status = jaccard_avx2_internal(a, b, n, &sums);
#elif defined(__ARM_FEATURE_SVE)
    hsd_log("CPU Path: SVE");
    status = jaccard_sve_internal(a, b, n, &sums);
#elif defined(__ARM_NEON)
    hsd_log("CPU Path: NEON");
    status = jaccard_neon_internal(a, b, n, &sums);
#else
    hsd_log("CPU Path: Scalar");
    status = jaccard_scalar_internal(a, b, n, &sums);
#endif

    if (status != HSD_SUCCESS) {
        hsd_log("CPU backend failed during sum calculation (status=%d).", status);
        return status;
    } else {
        hsd_log("CPU backend sum calculation succeeded.");
        status = calculate_jaccard_distance_from_sums_u64(sums.dot_product, sums.norm_a_sq,
                                                          sums.norm_b_sq, result);
        if (status != HSD_SUCCESS) {
            hsd_log("Final Jaccard distance calculation failed (status=%d).", status);
        }
    }

    hsd_log("Exit hsd_jaccard_u16 (final status=%d)", status);
    return status;
}
