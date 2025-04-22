#!/usr/bin/env python
import sys
import numpy as np
import hsdpy

VECTOR_DIM = 5

def print_system_info():
    """Prints available HsdPy library and system information."""
    print("--- Library Info ---")
    try:
        # Use the get_library_info function which includes backend info
        info = hsdpy.get_library_info()
        print(f"Library Path:   {info.get('lib_path', 'N/A')}")
        print(f"System:         {info.get('system', 'N/A')}")
        print(f"Architecture:   {info.get('arch', 'N/A')}")
        print(f"Active Backend: {info.get('backend', 'N/A')}")

    except Exception as e:
        print(f"Could not retrieve full library info: {e}", file=sys.stderr)
        # Fallback to just getting the backend if info failed
        try:
            backend = hsdpy.get_backend()
            print(f"Active Backend (fallback): {backend}")
        except Exception as be:
            print(f"Could not retrieve backend info: {be}", file=sys.stderr)

    print("---")


def run_and_print(description, func, *args, is_integer_result=False):
    """
    Runs an HsdPy function, prints the result or catches/prints errors.

    Args:
        description (str): Text description of the operation.
        func (callable): The HsdPy function to call.
        *args: Arguments to pass to the HsdPy function (e.g., numpy arrays).
        is_integer_result (bool): True if the result should be formatted as an integer.
    """
    try:
        result = func(*args)
        if is_integer_result:
            # Default int formatting is usually sufficient
            print(f"{description}: {result}")
        else:
            # Format floats similar to the C example
            print(f"{description}: {result:.4f}")
    except hsdpy.HsdError as e:
        print(f"ERROR: {description} failed - HsdError Status={e.status_code}, Msg='{e.message}'", file=sys.stderr)
    except NotImplementedError as e:
        print(f"ERROR: {description} failed - Function not available in C library. {e}", file=sys.stderr)
    except Exception as e:
        # Catch other potential errors like TypeError, ValueError from validation
        print(f"ERROR: {description} failed - {type(e).__name__}: {e}", file=sys.stderr)


def main():
    """Main function to demonstrate HsdPy API."""

    # --- Vector Definitions ---
    # Float vectors (for Euclidean, Manhattan, Cosine, Dot)
    vec_a_f32 = np.array([1.0, 2.0, 3.0, 4.0, 5.0], dtype=np.float32)
    vec_b_f32 = np.array([5.0, 4.0, 3.0, 2.0, 1.0], dtype=np.float32)

    # Binary vectors (uint8 for Hamming)
    vec_a_bin_u8 = np.array([1, 1, 0, 1, 0], dtype=np.uint8)
    vec_b_bin_u8 = np.array([1, 0, 1, 1, 1], dtype=np.uint8)

    # Binary-style vectors (uint16 for Jaccard - binary case)
    vec_a_bin_u16 = np.array([1, 1, 0, 1, 0], dtype=np.uint16)
    vec_b_bin_u16 = np.array([1, 0, 1, 1, 1], dtype=np.uint16)

    # Weighted/non-binary vectors (uint16 for Jaccard/Tanimoto - non-binary case)
    vec_a_weighted_u16 = np.array([3, 5, 0, 2, 0], dtype=np.uint16)
    vec_b_weighted_u16 = np.array([1, 5, 4, 2, 3], dtype=np.uint16)

    # --- Show System Info ---
    print_system_info()

    # --- Run Calculations ---
    print("\n--- Calculations (Using Auto-Detected Backend) ---")

    # Distance Metrics
    run_and_print("Squared Euclidean Distance (f32)",
                  hsdpy.dist_sqeuclidean_f32, vec_a_f32, vec_b_f32)
    run_and_print("Manhattan Distance (f32)",
                  hsdpy.dist_manhattan_f32, vec_a_f32, vec_b_f32)
    run_and_print("Hamming Distance (u8 binary)",
                  hsdpy.dist_hamming_u8, vec_a_bin_u8, vec_b_bin_u8,
                  is_integer_result=True)

    # Similarity Measures
    run_and_print("Dot Product Similarity (f32)",
                  hsdpy.sim_dot_f32, vec_a_f32, vec_b_f32)
    run_and_print("Cosine Similarity (f32)",
                  hsdpy.sim_cosine_f32, vec_a_f32, vec_b_f32)

    # Jaccard/Tanimoto Examples
    run_and_print("Jaccard Similarity (u16 binary input)",
                  hsdpy.sim_jaccard_u16, vec_a_bin_u16, vec_b_bin_u16)
    run_and_print("Tanimoto Coefficient (u16 non-binary input)",
                  hsdpy.sim_jaccard_u16, vec_a_weighted_u16, vec_b_weighted_u16)

    # --- Backend Selection ---
    print("\n--- Backend Selection ---")
    print(f"The active backend detected was: {hsdpy.get_backend()}")


if __name__ == "__main__":
    main()
