#ifndef HSDLIB_H
#define HSDLIB_H

#ifdef HSDLIB_NO_CHECKS
#define HSD_ALLOW_FP_CHECKS 0
#else
#define HSD_ALLOW_FP_CHECKS 1
#endif

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define HSD_ASM __asm__ volatile
#else
#define HSD_ASM asm volatile
#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

/**
 * @brief Status codes returned by Hsdlib functions.
 *
 * Most public functions return an hsd_status_t which indicates success or
 * a particular error condition. Callers should check for HSD_SUCCESS
 * (0) before using output values.
 */
typedef enum {
    HSD_SUCCESS = 0,               /**< Operation completed successfully */
    HSD_ERR_NULL_PTR = -1,         /**< A required pointer argument was NULL */
    HSD_ERR_INVALID_INPUT = -3,    /**< Input contained NaN, Inf, or otherwise invalid values */
    HSD_ERR_CPU_NOT_SUPPORTED = -4,/**< Requested backend is not supported on this machine */
    HSD_FAILURE = -99              /**< Generic failure */
} HSD_Status;

typedef HSD_Status hsd_status_t;

/**
 * @brief Floating-point status flags indicating FTZ/DAZ mode.
 */
typedef struct {
    bool ftz_enabled; /**< Flush-To-Zero enabled */
    bool daz_enabled; /**< Denormals-Are-Zero enabled */
} hsd_fp_status_t;

/**
 * @brief Backend selection constants for runtime dispatch.
 *
 * These values control which internal implementation (scalar/AVX/NEON/etc.)
 * is selected by the library. Use hsd_set_manual_backend() to request a
 * specific backend; by default, AUTO is used, and the best available backend
 * is selected at runtime.
 */
typedef enum {
    HSD_BACKEND_AUTO = 0,          /**< Auto-select the best backend */
    HSD_BACKEND_SCALAR,            /**< Portable scalar implementation */
    HSD_BACKEND_AVX,               /**< AVX implementation (x86) */
    HSD_BACKEND_AVX2,              /**< AVX2 implementation (x86) */
    HSD_BACKEND_AVX512F,           /**< AVX-512 (foundation) implementation (x86) */
    HSD_BACKEND_AVX512BW,          /**< AVX-512 BW support (x86) */
    HSD_BACKEND_AVX512DQ,          /**< AVX-512 DQ support (x86) */
    HSD_BACKEND_AVX512VPOPCNTDQ,   /**< AVX-512 VPOPCNTDQ support (x86) */
    HSD_BACKEND_NEON,              /**< NEON implementation (ARM) */
    HSD_BACKEND_SVE                /**< SVE implementation (ARM SVE) */
} HSD_Backend;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute squared Euclidean distance between two float vectors.
 *
 * @param a Pointer to the first input vector (length n)
 * @param b Pointer to the second input vector (length n)
 * @param n Number of elements in each vector
 * @param result Pointer to float where the result will be stored
 * @return hsd_status_t HSD_SUCCESS on success, otherwise an error code
 *
 * The function validates inputs (if HSD_ALLOW_FP_CHECKS is enabled) and
 * dispatches to the best available implementation for the current CPU.
 */
hsd_status_t hsd_dist_sqeuclidean_f32(const float* a, const float* b, size_t n, float* result);

/**
 * @brief Compute Manhattan distance (L1) between two float vectors.
 *
 * @param a Pointer to the first input vector (length n)
 * @param b Pointer to the second input vector (length n)
 * @param n Number of elements in each vector
 * @param result Pointer to float where the result will be stored
 * @return hsd_status_t HSD_SUCCESS on success, otherwise an error code
 */
hsd_status_t hsd_dist_manhattan_f32(const float* a, const float* b, size_t n, float* result);

/**
 * @brief Compute Hamming distance between two byte arrays (uint8_t).
 *
 * The Hamming distance counts bit differences; the returned value is the
 * total number of differing bits across all bytes in the vectors. The
 * result is written to the provided uint64_t pointer.
 *
 * @param a Pointer to first input array (length n)
 * @param b Pointer to second input array (length n)
 * @param n Number of elements in each array (in bytes)
 * @param result Pointer to uint64_t where the bit-difference count will be written
 * @return hsd_status_t HSD_SUCCESS on success, otherwise an error code
 */
hsd_status_t hsd_dist_hamming_u8(const uint8_t* a, const uint8_t* b, size_t n, uint64_t* result);

/**
 * @brief Compute dot product for float vectors.
 *
 * @param a Pointer to first input vector
 * @param b Pointer to second input vector
 * @param n Number of elements
 * @param result Pointer to float where result will be stored
 * @return hsd_status_t status code
 */
hsd_status_t hsd_sim_dot_f32(const float* a, const float* b, size_t n, float* result);

/**
 * @brief Compute cosine similarity for float vectors.
 *
 * Returns cosine similarity in [-1, 1]. For zero-length vectors the
 * behavior is defined by the implementation (often returns 1.0).
 *
 * @param a Pointer to first input vector
 * @param b Pointer to second input vector
 * @param n Number of elements
 * @param result Pointer to float where result will be stored
 * @return hsd_status_t status code
 */
hsd_status_t hsd_sim_cosine_f32(const float* a, const float* b, size_t n, float* result);

/**
 * @brief Compute Jaccard similarity for uint16_t vectors.
 *
 * The function treats elements as counts and computes a Jaccard-like
 * similarity measure. Result is a float in [0, 1].
 *
 * @param a Pointer to the first input array (uint16_t)
 * @param b Pointer to the second input array (uint16_t)
 * @param n Number of elements
 * @param result Pointer to float where result will be stored
 * @return hsd_status_t status code
 */
hsd_status_t hsd_sim_jaccard_u16(const uint16_t* a, const uint16_t* b, size_t n, float* result);

/**
 * @brief Get a human-readable description of the selected backend.
 *
 * Returns a pointer to a NUL-terminated string owned by the library. Do not
 * free the returned pointer.
 *
 * @return const char* NUL-terminated UTF-8 string describing the backend
 */
const char* hsd_get_backend(void);

/**
 * @brief Query if AVX-512 is available on this platform.
 *
 * @return true if AVX-512 capable, false otherwise
 */
bool hsd_has_avx512(void);

/**
 * @brief Query CPU floating-point mode (FTZ/DAZ status).
 *
 * @return hsd_fp_status_t structure describing FTZ/DAZ flags
 */
hsd_fp_status_t hsd_get_fp_mode_status(void);

/**
 * @brief Set manual backend selection.
 *
 * Requests the library use the specified backend instead of auto-dispatch.
 * Use HSD_BACKEND_AUTO to return to automatic selection.
 *
 * @param backend Backend enum value to set
 * @return hsd_status_t HSD_SUCCESS on success, error otherwise
 */
hsd_status_t hsd_set_manual_backend(HSD_Backend backend);

/**
 * @brief Get the current backend choice (auto or manual selection).
 *
 * @return HSD_Backend current backend choice
 */
HSD_Backend hsd_get_current_backend_choice(void);

#if defined(__x86_64__) || defined(_M_X64)
/**
 * @name x86 feature detection helpers
 * Helper functions that return whether specific instruction set features are available.
 */
/**@{*/
bool hsd_cpu_has_avx(void);
bool hsd_cpu_has_avx2(void);
bool hsd_cpu_has_fma(void);
bool hsd_cpu_has_avx512f(void);
bool hsd_cpu_has_avx512bw(void);
bool hsd_cpu_has_avx512dq(void);
bool hsd_cpu_has_avx512vpopcntdq(void);
/**@}*/
#elif defined(__aarch64__)
/**
 * @name ARM feature detection helpers
 */
/**@{*/
bool hsd_cpu_has_neon(void);
bool hsd_cpu_has_sve(void);
/**@}*/
#endif

#ifdef HSD_DEBUG
#include <stdarg.h>
#include <stdio.h>
static inline void hsdlib_internal_do_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[HSDLIB_DEBUG] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(stderr);
}
#define hsd_log(...) hsdlib_internal_do_log(__VA_ARGS__)
#else
#define hsd_log(...) ((void)0)
#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>

static inline float hsd_internal_hsum_avx_f32(__m256 acc) {
    __m128 hsum_128 = _mm_add_ps(_mm256_castps256_ps128(acc), _mm256_extractf128_ps(acc, 1));
    hsum_128 = _mm_hadd_ps(hsum_128, hsum_128);
    hsum_128 = _mm_hadd_ps(hsum_128, hsum_128);
    return _mm_cvtss_f32(hsum_128);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
