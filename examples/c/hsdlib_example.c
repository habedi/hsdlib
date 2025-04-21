#include <hsdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define VECTOR_DIM 5

typedef struct {
    const char* name;
    hsd_status_t status;
    union {
        float f32;
        uint64_t u64;
    } result;
} operation_result_t;

void print_operation_result(operation_result_t* op, bool is_integer) {
    if (op->status == HSD_SUCCESS) {
        if (is_integer) {
            printf("%s: %llu\n", op->name, (unsigned long long)op->result.u64);
        } else {
            printf("%s: %.4f\n", op->name, op->result.f32);
        }
    } else {
        printf("%s failed with status: %d\n", op->name, op->status);
    }
}

void print_system_info() {
    printf("--- Library Info ---\n");
    printf("Backend String: %s\n", hsd_get_backend());
    printf("Has AVX512 (int): %d\n", hsd_has_avx512());

    hsd_fp_status_t fp_status = hsd_get_fp_mode_status();
    printf("Floating Point Mode: FTZ=%d, DAZ=%d\n", fp_status.ftz_enabled, fp_status.daz_enabled);
    printf("Current Backend Choice (Enum): %d\n", hsd_get_current_backend_choice());

#if defined(__x86_64__) || defined(_M_X64)
    printf(
        "CPU Features: AVX=%d AVX2=%d FMA=%d AVX512F=%d AVX512BW=%d AVX512DQ=%d "
        "AVX512VPOPCNTDQ=%d\n",
        hsd_cpu_has_avx(), hsd_cpu_has_avx2(), hsd_cpu_has_fma(), hsd_cpu_has_avx512f(),
        hsd_cpu_has_avx512bw(), hsd_cpu_has_avx512dq(), hsd_cpu_has_avx512vpopcntdq());
#elif defined(__aarch64__)
    printf("CPU Features: NEON=%d SVE=%d\n", hsd_cpu_has_neon(), hsd_cpu_has_sve());
#else
    printf("CPU Feature checks not available for this architecture.\n");
#endif
}

int main() {
    // Float vectors (for (squared) Euclidean, Manhattan, etc.)
    float vec_a_f32[VECTOR_DIM] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float vec_b_f32[VECTOR_DIM] = {5.0f, 4.0f, 3.0f, 2.0f, 1.0f};

    // Binary vectors (for Hamming distance)
    uint8_t vec_a_bin_u8[VECTOR_DIM] = {1, 1, 0, 1, 0};
    uint8_t vec_b_bin_u8[VECTOR_DIM] = {1, 0, 1, 1, 1};

    // Binary-style vectors (for Jaccard similarity)
    uint16_t vec_a_bin_u16[VECTOR_DIM] = {1, 1, 0, 1, 0};
    uint16_t vec_b_bin_u16[VECTOR_DIM] = {1, 0, 1, 1, 1};

    // Calculation expectation for non-binary vectors (Tanimoto coefficient)
    uint16_t vec_a_weighted_u16[VECTOR_DIM] = {3, 5, 0, 2, 0};
    uint16_t vec_b_weighted_u16[VECTOR_DIM] = {1, 5, 4, 2, 3};

    operation_result_t op_result;

    // Show system information
    print_system_info();

    printf("\n--- Calculations (Auto Backend) ---\n");

    // Distance metrics
    op_result.name = "Squared Euclidean Distance (f32)";
    op_result.status =
        hsd_dist_sqeuclidean_f32(vec_a_f32, vec_b_f32, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    op_result.name = "Manhattan Distance (f32)";
    op_result.status =
        hsd_dist_manhattan_f32(vec_a_f32, vec_b_f32, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    op_result.name = "Hamming Distance (u8 binary)";
    op_result.status =
        hsd_dist_hamming_u8(vec_a_bin_u8, vec_b_bin_u8, VECTOR_DIM, &op_result.result.u64);
    print_operation_result(&op_result, true);

    // Similarity measures
    op_result.name = "Dot Product Similarity (f32)";
    op_result.status = hsd_sim_dot_f32(vec_a_f32, vec_b_f32, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    op_result.name = "Cosine Similarity (f32)";
    op_result.status = hsd_sim_cosine_f32(vec_a_f32, vec_b_f32, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    // --- Jaccard Examples ---
    op_result.name = "Jaccard Similarity (u16 binary input)";
    op_result.status =
        hsd_sim_jaccard_u16(vec_a_bin_u16, vec_b_bin_u16, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    op_result.name = "Tanimoto coefficient (u16 non-binary input)";
    op_result.status = hsd_sim_jaccard_u16(vec_a_weighted_u16, vec_b_weighted_u16, VECTOR_DIM,
                                           &op_result.result.f32);
    print_operation_result(&op_result, false);
    // --- End Jaccard Examples ---

    // Demonstrate manual backend selection
    printf("\n--- Calculations (Forced Scalar Backend) ---\n");
    hsd_status_t status = hsd_set_manual_backend(HSD_BACKEND_SCALAR);
    if (status != HSD_SUCCESS) {
        printf("Failed to set manual backend SCALAR.\n");
        return 1;
    }
    printf("Manually set backend to SCALAR. New Backend String: %s\n", hsd_get_backend());

    // Rerun one calculation with scalar backend
    op_result.name = "Squared Euclidean Distance (f32, forced scalar)";
    op_result.status =
        hsd_dist_sqeuclidean_f32(vec_a_f32, vec_b_f32, VECTOR_DIM, &op_result.result.f32);
    print_operation_result(&op_result, false);

    // Reset backend to auto
    hsd_set_manual_backend(HSD_BACKEND_AUTO);
    printf("\nBackend set back to AUTO: %s\n", hsd_get_backend());

    return 0;
}
