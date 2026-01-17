/**
 * @file test_shuffle.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for Feistel permutation
 *
 * @traceability SRS-004-SHUFFLE, CT-MATH-001 §7
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ct_types.h"
#include "shuffle.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Known Test Vectors (CT-MATH-001 §7.2)
 * ============================================================================ */

static int test_vector_N100_idx0(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 0;
    uint32_t N = 100;
    
    /* CT-MATH-001 test vector: N=100, index=0 → 26 */
    uint32_t result = ct_permute_index(0, N, seed, epoch);
    
    return result == 26;
}

static int test_vector_N100_idx99(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 0;
    uint32_t N = 100;
    
    /* CT-MATH-001 test vector: N=100, index=99 → 41 */
    uint32_t result = ct_permute_index(99, N, seed, epoch);
    
    return result == 41;
}

static int test_vector_N100_epoch1(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 1;
    uint32_t N = 100;
    
    /* CT-MATH-001 test vector: N=100, epoch=1, index=0 → 66 */
    uint32_t result = ct_permute_index(0, N, seed, epoch);
    
    return result == 66;
}

static int test_vector_N60000_idx0(void)
{
    uint64_t seed = 0xFEDCBA9876543210ULL;
    uint32_t epoch = 0;
    uint32_t N = 60000;
    
    /* CT-MATH-001 test vector: N=60000, index=0 → 26382 */
    uint32_t result = ct_permute_index(0, N, seed, epoch);
    
    return result == 26382;
}

static int test_vector_N60000_idx59999(void)
{
    uint64_t seed = 0xFEDCBA9876543210ULL;
    uint32_t epoch = 0;
    uint32_t N = 60000;
    
    /* CT-MATH-001 test vector: N=60000, index=59999 → 20774 */
    uint32_t result = ct_permute_index(59999, N, seed, epoch);
    
    return result == 20774;
}

/* ============================================================================
 * Test: Bijection Property
 * ============================================================================ */

static int test_bijection_N100(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 0;
    uint32_t N = 100;
    
    uint8_t seen[100] = {0};
    
    /* Permute all indices */
    for (uint32_t i = 0; i < N; i++) {
        uint32_t result = ct_permute_index(i, N, seed, epoch);
        
        /* Must be in valid range */
        if (result >= N) return 0;
        
        /* Must not be duplicate */
        if (seen[result] != 0) return 0;
        
        seen[result] = 1;
    }
    
    /* Verify all outputs seen exactly once */
    for (uint32_t i = 0; i < N; i++) {
        if (seen[i] != 1) return 0;
    }
    
    return 1;
}

static int test_bijection_N1000(void)
{
    uint64_t seed = 0xABCDEF0123456789ULL;
    uint32_t epoch = 5;
    uint32_t N = 1000;
    
    uint8_t *seen = calloc(N, sizeof(uint8_t));
    if (!seen) return 0;
    
    int result_ok = 1;
    
    for (uint32_t i = 0; i < N; i++) {
        uint32_t result = ct_permute_index(i, N, seed, epoch);
        
        if (result >= N || seen[result] != 0) {
            result_ok = 0;
            break;
        }
        
        seen[result] = 1;
    }
    
    if (result_ok) {
        for (uint32_t i = 0; i < N; i++) {
            if (seen[i] != 1) {
                result_ok = 0;
                break;
            }
        }
    }
    
    free(seen);
    return result_ok;
}

static int test_bijection_power_of_two(void)
{
    /* N = power of 2 (special case) */
    uint64_t seed = 0x1111111111111111ULL;
    uint32_t epoch = 0;
    uint32_t N = 256;
    
    uint8_t seen[256] = {0};
    
    for (uint32_t i = 0; i < N; i++) {
        uint32_t result = ct_permute_index(i, N, seed, epoch);
        if (result >= N || seen[result] != 0) return 0;
        seen[result] = 1;
    }
    
    for (uint32_t i = 0; i < N; i++) {
        if (seen[i] != 1) return 0;
    }
    
    return 1;
}

static int test_bijection_prime(void)
{
    /* N = prime number */
    uint64_t seed = 0x2222222222222222ULL;
    uint32_t epoch = 0;
    uint32_t N = 97;  /* Prime */
    
    uint8_t seen[97] = {0};
    
    for (uint32_t i = 0; i < N; i++) {
        uint32_t result = ct_permute_index(i, N, seed, epoch);
        if (result >= N || seen[result] != 0) return 0;
        seen[result] = 1;
    }
    
    return 1;
}

/* ============================================================================
 * Test: Determinism
 * ============================================================================ */

static int test_determinism_same_inputs(void)
{
    uint64_t seed = 0x9876543210FEDCBAULL;
    uint32_t epoch = 10;
    uint32_t N = 500;
    
    uint32_t r1 = ct_permute_index(250, N, seed, epoch);
    uint32_t r2 = ct_permute_index(250, N, seed, epoch);
    
    return r1 == r2;
}

static int test_determinism_sequence(void)
{
    uint64_t seed = 0xAAAAAAAAAAAAAAAAULL;
    uint32_t epoch = 0;
    uint32_t N = 200;
    
    uint32_t seq1[200];
    uint32_t seq2[200];
    
    /* Generate sequence twice */
    for (uint32_t i = 0; i < N; i++) {
        seq1[i] = ct_permute_index(i, N, seed, epoch);
    }
    
    for (uint32_t i = 0; i < N; i++) {
        seq2[i] = ct_permute_index(i, N, seed, epoch);
    }
    
    /* Must be identical */
    return memcmp(seq1, seq2, N * sizeof(uint32_t)) == 0;
}

/* ============================================================================
 * Test: Range Validity
 * ============================================================================ */

static int test_range_all_outputs_valid(void)
{
    uint64_t seed = 0xBBBBBBBBBBBBBBBBULL;
    uint32_t epoch = 0;
    uint32_t N = 300;
    
    for (uint32_t i = 0; i < N; i++) {
        uint32_t result = ct_permute_index(i, N, seed, epoch);
        if (result >= N) return 0;
    }
    
    return 1;
}

static int test_range_N_one(void)
{
    /* N=1 should always map to 0 */
    uint32_t result = ct_permute_index(0, 1, 12345, 0);
    return result == 0;
}

static int test_range_out_of_bounds_input(void)
{
    /* Input >= N should be handled gracefully */
    uint32_t N = 100;
    uint32_t result = ct_permute_index(150, N, 12345, 0);
    
    /* Should clamp to valid range */
    return result < N;
}

/* ============================================================================
 * Test: Context Initialization
 * ============================================================================ */

static int test_ctx_init(void)
{
    ct_shuffle_ctx_t ctx;
    uint64_t seed = 0x1234567890ABCDEFULL;
    uint32_t epoch = 42;
    
    ct_shuffle_init(&ctx, seed, epoch);
    
    if (ctx.seed != seed) return 0;
    if (ctx.epoch != epoch) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Verification Function
 * ============================================================================ */

static int test_verify_valid_bijection(void)
{
    uint64_t seed = 0xFEDCBA9876543210ULL;
    uint32_t epoch = 0;
    uint32_t N = 100;
    
    int valid = ct_shuffle_verify_bijection(seed, epoch, N, N);
    
    return valid == 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - Feistel Shuffle Tests\n");
    printf("Traceability: SRS-004-SHUFFLE, CT-MATH-001 §7\n");
    printf("==============================================\n\n");
    
    printf("Known test vectors (CT-MATH-001 §7.2):\n");
    RUN_TEST(test_vector_N100_idx0);
    RUN_TEST(test_vector_N100_idx99);
    RUN_TEST(test_vector_N100_epoch1);
    RUN_TEST(test_vector_N60000_idx0);
    RUN_TEST(test_vector_N60000_idx59999);
    
    printf("\nBijection property:\n");
    RUN_TEST(test_bijection_N100);
    RUN_TEST(test_bijection_N1000);
    RUN_TEST(test_bijection_power_of_two);
    RUN_TEST(test_bijection_prime);
    
    printf("\nDeterminism:\n");
    RUN_TEST(test_determinism_same_inputs);
    RUN_TEST(test_determinism_sequence);
    
    printf("\nRange validity:\n");
    RUN_TEST(test_range_all_outputs_valid);
    RUN_TEST(test_range_N_one);
    RUN_TEST(test_range_out_of_bounds_input);
    
    printf("\nContext initialization:\n");
    RUN_TEST(test_ctx_init);
    
    printf("\nVerification function:\n");
    RUN_TEST(test_verify_valid_bijection);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
