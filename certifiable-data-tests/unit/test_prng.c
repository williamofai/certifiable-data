/**
 * @file test_prng.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for deterministic PRNG
 *
 * @traceability CT-MATH-001 §5
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include "ct_types.h"
#include "prng.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Determinism - Core Property
 * ============================================================================ */

static int test_same_inputs_same_output(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 5;
    uint32_t op_id = 42;
    
    uint64_t r1 = ct_prng(seed, epoch, op_id);
    uint64_t r2 = ct_prng(seed, epoch, op_id);
    
    return r1 == r2;
}

static int test_different_seed_different_output(void)
{
    uint32_t epoch = 0;
    uint32_t op_id = 0;
    
    uint64_t r1 = ct_prng(0x1111111111111111ULL, epoch, op_id);
    uint64_t r2 = ct_prng(0x2222222222222222ULL, epoch, op_id);
    
    return r1 != r2;
}

static int test_different_epoch_different_output(void)
{
    uint64_t seed = 0xABCDEF0123456789ULL;
    uint32_t op_id = 0;
    
    uint64_t r1 = ct_prng(seed, 0, op_id);
    uint64_t r2 = ct_prng(seed, 1, op_id);
    
    return r1 != r2;
}

static int test_different_opid_different_output(void)
{
    uint64_t seed = 0xFEDCBA9876543210ULL;
    uint32_t epoch = 0;
    
    uint64_t r1 = ct_prng(seed, epoch, 100);
    uint64_t r2 = ct_prng(seed, epoch, 101);
    
    return r1 != r2;
}

/* ============================================================================
 * Test: Distribution Quality
 * ============================================================================ */

static int test_not_zero(void)
{
    /* PRNG should not always return zero */
    int found_nonzero = 0;
    
    for (uint32_t i = 0; i < 100; i++) {
        uint64_t r = ct_prng(i, 0, 0);
        if (r != 0) {
            found_nonzero = 1;
            break;
        }
    }
    
    return found_nonzero;
}

static int test_not_constant(void)
{
    uint64_t first = ct_prng(1, 0, 0);
    
    for (uint32_t i = 1; i < 100; i++) {
        uint64_t r = ct_prng(1, 0, i);
        if (r != first) return 1;
    }
    
    return 0;  /* All same - bad */
}

static int test_avalanche_seed(void)
{
    /* Changing one bit in seed should change many bits in output */
    uint64_t r1 = ct_prng(0x0000000000000000ULL, 0, 0);
    uint64_t r2 = ct_prng(0x0000000000000001ULL, 0, 0);
    
    uint64_t diff = r1 ^ r2;
    
    /* Count bits that differ */
    int bit_count = 0;
    for (int i = 0; i < 64; i++) {
        if (diff & (1ULL << i)) bit_count++;
    }
    
    /* Expect at least 20 bits to differ (good avalanche) */
    return bit_count >= 20;
}

/* ============================================================================
 * Test: Uniform Distribution
 * ============================================================================ */

static int test_uniform_range(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t n = 100;
    
    /* All values should be in [0, n) */
    for (uint32_t i = 0; i < 1000; i++) {
        uint32_t val = ct_prng_uniform(seed, 0, i, n);
        if (val >= n) return 0;
    }
    
    return 1;
}

static int test_uniform_n_zero(void)
{
    /* n=0 should return 0 */
    uint32_t result = ct_prng_uniform(12345, 0, 0, 0);
    return result == 0;
}

static int test_uniform_n_one(void)
{
    /* n=1 should always return 0 */
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t result = ct_prng_uniform(i, 0, 0, 1);
        if (result != 0) return 0;
    }
    return 1;
}

static int test_uniform_deterministic(void)
{
    uint64_t seed = 0xABCDEF0123456789ULL;
    uint32_t epoch = 5;
    uint32_t n = 50;
    
    uint32_t r1 = ct_prng_uniform(seed, epoch, 10, n);
    uint32_t r2 = ct_prng_uniform(seed, epoch, 10, n);
    
    return r1 == r2;
}

static int test_uniform_coverage(void)
{
    /* Over many samples, should hit all values in small range */
    uint64_t seed = 0x9999999999999999ULL;
    uint32_t n = 10;
    uint8_t seen[10] = {0};
    
    for (uint32_t i = 0; i < 1000; i++) {
        uint32_t val = ct_prng_uniform(seed, 0, i, n);
        if (val < n) seen[val] = 1;
    }
    
    /* Should have seen all values */
    for (uint32_t i = 0; i < n; i++) {
        if (!seen[i]) return 0;
    }
    
    return 1;
}

/* ============================================================================
 * Test: Known Test Vectors (CT-MATH-001 §5)
 * ============================================================================ */

static int test_known_vectors(void)
{
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 0;
    
    /* Generate test vectors for cross-platform verification */
    uint64_t v0 = ct_prng(seed, epoch, 0);
    uint64_t v1 = ct_prng(seed, epoch, 1);
    uint64_t v2 = ct_prng(seed, epoch, 2);
    
    printf("\n    PRNG test vectors (seed=0x123456789ABCDEF0, epoch=0):\n");
    printf("      op_id=0: 0x%016llX\n", (unsigned long long)v0);
    printf("      op_id=1: 0x%016llX\n", (unsigned long long)v1);
    printf("      op_id=2: 0x%016llX\n", (unsigned long long)v2);
    
    /* TODO: After establishing reference platform, verify exact values */
    
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - PRNG Tests\n");
    printf("Traceability: CT-MATH-001 §5\n");
    printf("==============================================\n\n");
    
    printf("Determinism (core property):\n");
    RUN_TEST(test_same_inputs_same_output);
    RUN_TEST(test_different_seed_different_output);
    RUN_TEST(test_different_epoch_different_output);
    RUN_TEST(test_different_opid_different_output);
    
    printf("\nDistribution quality:\n");
    RUN_TEST(test_not_zero);
    RUN_TEST(test_not_constant);
    RUN_TEST(test_avalanche_seed);
    
    printf("\nUniform distribution:\n");
    RUN_TEST(test_uniform_range);
    RUN_TEST(test_uniform_n_zero);
    RUN_TEST(test_uniform_n_one);
    RUN_TEST(test_uniform_deterministic);
    RUN_TEST(test_uniform_coverage);
    
    printf("\nKnown test vectors:\n");
    RUN_TEST(test_known_vectors);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
