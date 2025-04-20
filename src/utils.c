#include <stdatomic.h>
#include <stdio.h>

#include "hsdlib.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <cpuid.h>
#include <immintrin.h>
#if !defined(__EMSCRIPTEN__)
#include <xmmintrin.h>
#endif
#elif defined(__aarch64__)
#if defined(__linux__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#include <arm_neon.h>
#endif

static atomic_flag hsd_features_checked_flag = ATOMIC_FLAG_INIT;

#if defined(__x86_64__) || defined(_M_X64)
static bool hsd_has_avx_ = false;
static bool hsd_has_avx2_ = false;
static bool hsd_has_fma_ = false;
static bool hsd_has_avx512f_ = false;
static bool hsd_has_avx512bw_ = false;
static bool hsd_has_avx512dq_ = false;  // <<< ADDED DQ flag
static bool hsd_has_avx512vpopcntdq_ = false;
#elif defined(__aarch64__)
static bool hsd_has_neon_ = false;
static bool hsd_has_sve_ = false;
#endif

static void hsd_check_cpu_features_internal(void) {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid_max(0, NULL)) return;
    unsigned int max_level = __get_cpuid_max(0, NULL);

    if (max_level >= 1) {
        __cpuid(1, eax, ebx, ecx, edx);

        hsd_has_fma_ = (ecx & bit_FMA);

        bool has_cpuid_avx = (ecx & bit_AVX);
        bool has_osxsave = (ecx & bit_OSXSAVE);
        if (has_cpuid_avx && has_osxsave) {
            unsigned int xcr0_lo, xcr0_hi;
            __asm__ volatile("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));

            hsd_has_avx_ = ((xcr0_lo & 0x6) == 0x6);
        } else {
            hsd_has_avx_ = false;
        }
    }
    if (max_level >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        hsd_has_avx2_ = (ebx & bit_AVX2);
        hsd_has_avx512f_ = (ebx & bit_AVX512F);
        hsd_has_avx512bw_ = (ebx & bit_AVX512BW);
        hsd_has_avx512dq_ = (ebx & bit_AVX512DQ);  // <<< ADDED Check for DQ (bit 17 in EBX)
        hsd_has_avx512vpopcntdq_ = (ecx & bit_AVX512VPOPCNTDQ);
    }

    // <<< UPDATED Log message
    hsd_log(
        "x86 Features: AVX=%d AVX2=%d FMA=%d AVX512F=%d AVX512BW=%d AVX512DQ=%d AVX512VPOPCNTDQ=%d",
        hsd_has_avx_, hsd_has_avx2_, hsd_has_fma_, hsd_has_avx512f_, hsd_has_avx512bw_,
        hsd_has_avx512dq_, hsd_has_avx512vpopcntdq_);

#elif defined(__aarch64__)
#if defined(__linux__)
    unsigned long hwcap = getauxval(AT_HWCAP);
    hsd_has_neon_ = (hwcap & HWCAP_ASIMD);
    hsd_has_sve_ = (hwcap & HWCAP_SVE);
#elif defined(__APPLE__)
    int has_feature = 0;
    size_t size = sizeof(has_feature);
    if (sysctlbyname("hw.optional.neon", &has_feature, &size, NULL, 0) == 0)
        hsd_has_neon_ = (bool)has_feature;
    else
        hsd_has_neon_ = true;
    if (sysctlbyname("hw.optional.arm.FEAT_SVE", &has_feature, &size, NULL, 0) == 0)
        hsd_has_sve_ = (bool)has_feature;
    else
        hsd_has_sve_ = false;
#else
    hsd_log("AArch64 runtime CPU feature detection not implemented for this OS.");
    hsd_has_neon_ = true;
#endif
    hsd_log("AArch64 Features: NEON=%d SVE=%d", hsd_has_neon_, hsd_has_sve_);
#else
    hsd_log("Runtime CPU feature detection not supported on this Arch.");
#endif
}

static inline void hsd_ensure_features_checked(void) {
    if (!atomic_flag_test_and_set_explicit(&hsd_features_checked_flag, memory_order_acquire)) {
        hsd_check_cpu_features_internal();
    }
}

#define DEFINE_HSD_CPU_CHECKER(feature) \
    bool hsd_cpu_has_##feature(void) {  \
        hsd_ensure_features_checked();  \
        return hsd_has_##feature##_;    \
    }

#if defined(__x86_64__) || defined(_M_X64)
DEFINE_HSD_CPU_CHECKER(avx)
DEFINE_HSD_CPU_CHECKER(avx2)
DEFINE_HSD_CPU_CHECKER(fma)
DEFINE_HSD_CPU_CHECKER(avx512f)
DEFINE_HSD_CPU_CHECKER(avx512bw)
DEFINE_HSD_CPU_CHECKER(avx512dq)  // <<< ADDED DQ checker definition
DEFINE_HSD_CPU_CHECKER(avx512vpopcntdq)
#elif defined(__aarch64__)
DEFINE_HSD_CPU_CHECKER(neon)
DEFINE_HSD_CPU_CHECKER(sve)
#endif

static atomic_int hsd_forced_backend = ATOMIC_VAR_INIT(HSD_BACKEND_AUTO);

hsd_status_t hsd_set_manual_backend(HSD_Backend backend) {
    hsd_log("Setting manual backend to: %d", backend);
    atomic_store_explicit(&hsd_forced_backend, backend, memory_order_release);
    return HSD_SUCCESS;
}

HSD_Backend hsd_get_current_backend_choice(void) {
    return (HSD_Backend)atomic_load_explicit(&hsd_forced_backend, memory_order_acquire);
}

const char *hsd_get_backend(void) {
    HSD_Backend forced = hsd_get_current_backend_choice();
    if (forced != HSD_BACKEND_AUTO) {
        // This switch likely doesn't need AVX512DQ explicitly,
        // as BW/VPOPCNTDQ imply DQ, but keeping it simple for now.
        switch (forced) {
            case HSD_BACKEND_SCALAR:
                return "Forced Scalar";
            case HSD_BACKEND_AVX:
                return "Forced AVX";
            case HSD_BACKEND_AVX2:
                return "Forced AVX2";
            case HSD_BACKEND_AVX512F:
                return "Forced AVX512F";
            case HSD_BACKEND_AVX512BW:
                return "Forced AVX512BW";
            // Maybe add DQ case if needed for forcing?
            case HSD_BACKEND_AVX512VPOPCNTDQ:
                return "Forced AVX512VPOPCNTDQ";
            case HSD_BACKEND_NEON:
                return "Forced NEON";
            case HSD_BACKEND_SVE:
                return "Forced SVE";
            default:
                return "Forced Unknown";
        }
    } else {
#if defined(__x86_64__) || defined(_M_X64)
        // Order matters - check most specific first
        if (hsd_cpu_has_avx512vpopcntdq())  // Implies F, BW, DQ
            return "Auto (AVX512VPOPCNTDQ Capable)";
        // else if (hsd_cpu_has_avx512dq()) // Could add DQ check here if needed
        //     return "Auto (AVX512DQ Capable)";
        else if (hsd_cpu_has_avx512bw())  // Implies F
            return "Auto (AVX512BW Capable)";
        else if (hsd_cpu_has_avx512f())
            return "Auto (AVX512F Capable)";
        else if (hsd_cpu_has_avx2())
            return "Auto (AVX2 Capable)";
        else if (hsd_cpu_has_avx())
            return "Auto (AVX Capable)";
        else
            return "Auto (Scalar/SSE)";
#elif defined(__aarch64__)
#if defined(__ARM_FEATURE_SVE)
        if (hsd_cpu_has_sve())
            return "Auto (SVE Capable)";
        else
#endif
            if (hsd_cpu_has_neon())
            return "Auto (NEON Capable)";
        else
            return "Auto (Scalar)";
#else
        return "Auto (Scalar)";
#endif
    }
}

int hsd_has_avx512(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return hsd_cpu_has_avx512f();
#else
    return 0;
#endif
}

hsd_fp_status_t hsd_get_fp_mode_status(void) {
    hsd_fp_status_t status = {-1, -1};
#if defined(__x86_64__) || defined(_M_X64)
#if !defined(__EMSCRIPTEN__)
#define HSD_MXCSR_FTZ_BIT (1 << 15)
#define HSD_MXCSR_DAZ_BIT (1 << 6)
    unsigned int mxcsr = _mm_getcsr();
    status.ftz_enabled = (mxcsr & HSD_MXCSR_FTZ_BIT) ? 1 : 0;
    status.daz_enabled = (mxcsr & HSD_MXCSR_DAZ_BIT) ? 1 : 0;
#undef HSD_MXCSR_FTZ_BIT
#undef HSD_MXCSR_DAZ_BIT
#endif
#elif defined(__aarch64__)
#define HSD_FPCR_FZ_BIT (1UL << 24)
    unsigned long fpcr_val;
    HSD_ASM("mrs %0, fpcr" : "=r"(fpcr_val));
    status.ftz_enabled = (fpcr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;
#undef HSD_FPCR_FZ_BIT
#elif defined(__arm__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
#define HSD_FPCR_FZ_BIT (1UL << 24)
    unsigned int fpscr_val;
    HSD_ASM("vmrs %0, fpscr" : "=r"(fpscr_val));
    status.ftz_enabled = (fpscr_val & HSD_FPCR_FZ_BIT) ? 1 : 0;
    status.daz_enabled = status.ftz_enabled;
#undef HSD_FPCR_FZ_BIT
#else
    hsd_log("Warning: Could not determine FTZ/DAZ status for this platform.");
#endif
    return status;
}
