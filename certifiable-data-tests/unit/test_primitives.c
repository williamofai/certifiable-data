/**
 * @file test_primitives.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for DVM primitives
 *
 * @traceability SRS-001-LOADER, CT-MATH-001 §3
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include "ct_types.h"
#include "dvm.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Clamp32
 * ============================================================================ */

static int test_clamp32_no_overflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_clamp32(12345, &faults);
    
    if (result != 12345) return 0;
    if (faults.overflow != 0) return 0;
    if (faults.underflow != 0) return 0;
    
    return 1;
}

static int test_clamp32_overflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int64_t big = (int64_t)INT32_MAX + 100;
    int32_t result = dvm_clamp32(big, &faults);
    
    if (result != INT32_MAX) return 0;
    if (faults.overflow != 1) return 0;
    
    return 1;
}

static int test_clamp32_underflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int64_t small = (int64_t)INT32_MIN - 100;
    int32_t result = dvm_clamp32(small, &faults);
    
    if (result != INT32_MIN) return 0;
    if (faults.underflow != 1) return 0;
    
    return 1;
}

static int test_clamp32_boundary_max(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_clamp32(INT32_MAX, &faults);
    
    if (result != INT32_MAX) return 0;
    if (faults.overflow != 0) return 0;
    
    return 1;
}

static int test_clamp32_boundary_min(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_clamp32(INT32_MIN, &faults);
    
    if (result != INT32_MIN) return 0;
    if (faults.underflow != 0) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Saturating Addition
 * ============================================================================ */

static int test_add32_normal(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_add32(100, 200, &faults);
    
    if (result != 300) return 0;
    if (faults.overflow != 0) return 0;
    
    return 1;
}

static int test_add32_overflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_add32(INT32_MAX, 1, &faults);
    
    if (result != INT32_MAX) return 0;
    if (faults.overflow != 1) return 0;
    
    return 1;
}

static int test_add32_underflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_add32(INT32_MIN, -1, &faults);
    
    if (result != INT32_MIN) return 0;
    if (faults.underflow != 1) return 0;
    
    return 1;
}

static int test_add32_negative(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_add32(-100, -200, &faults);
    
    if (result != -300) return 0;
    if (faults.underflow != 0) return 0;
    
    return 1;
}

static int test_add32_mixed_sign(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_add32(100, -50, &faults);
    
    if (result != 50) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Saturating Subtraction
 * ============================================================================ */

static int test_sub32_normal(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_sub32(300, 100, &faults);
    
    if (result != 200) return 0;
    if (faults.underflow != 0) return 0;
    
    return 1;
}

static int test_sub32_overflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_sub32(INT32_MAX, -1, &faults);
    
    if (result != INT32_MAX) return 0;
    if (faults.overflow != 1) return 0;
    
    return 1;
}

static int test_sub32_underflow(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_sub32(INT32_MIN, 1, &faults);
    
    if (result != INT32_MIN) return 0;
    if (faults.underflow != 1) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: 64-bit Multiply (no overflow)
 * ============================================================================ */

static int test_mul64_positive(void)
{
    int64_t result = dvm_mul64(1000, 2000);
    
    return result == 2000000LL;
}

static int test_mul64_negative(void)
{
    int64_t result = dvm_mul64(-1000, 2000);
    
    return result == -2000000LL;
}

static int test_mul64_zero(void)
{
    int64_t result = dvm_mul64(12345, 0);
    
    return result == 0;
}

static int test_mul64_max(void)
{
    /* Max 32-bit values should multiply without overflow in 64-bit */
    int64_t result = dvm_mul64(INT32_MAX, 2);
    
    return result == ((int64_t)INT32_MAX * 2);
}

/* ============================================================================
 * Test: Round-to-Nearest-Even (RNE) - CT-MATH-001 §3.5
 * ============================================================================ */

static int test_rne_test_vector_1(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Test vector 1: 1.5 rounds to 2 (even) - CT-MATH-001 */
    int32_t result = dvm_round_shift_rne(0x00018000, 16, &faults);
    
    if (result != 2) return 0;
    if (faults.overflow != 0) return 0;
    
    return 1;
}

static int test_rne_test_vector_2(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Test vector 2: 2.5 rounds to 2 (even) - CT-MATH-001 */
    int32_t result = dvm_round_shift_rne(0x00028000, 16, &faults);
    
    return result == 2;
}

static int test_rne_test_vector_3(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Test vector 3: 3.5 rounds to 4 (even) - CT-MATH-001 */
    int32_t result = dvm_round_shift_rne(0x00038000, 16, &faults);
    
    return result == 4;
}

static int test_rne_negative(void)
{
    ct_fault_flags_t faults = {0};
    
    /* -1.5 should round to -2 (even) */
    int32_t result = dvm_round_shift_rne((int64_t)0xFFFFFFFFFFFE8000LL, 16, &faults);
    
    return result == -2;
}

static int test_rne_round_down(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 1.25 should round down to 1 */
    int32_t result = dvm_round_shift_rne(0x00014000, 16, &faults);
    
    return result == 1;
}

static int test_rne_round_up(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 1.75 should round up to 2 */
    int32_t result = dvm_round_shift_rne(0x0001C000, 16, &faults);
    
    return result == 2;
}

static int test_rne_shift_zero(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_round_shift_rne(12345, 0, &faults);
    
    return result == 12345;
}

static int test_rne_shift_bounds(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Shift > 62 should fault */
    int32_t result = dvm_round_shift_rne(12345, 63, &faults);
    
    if (result != 0) return 0;
    if (faults.domain != 1) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Q16.16 Fixed-Point Multiply
 * ============================================================================ */

static int test_mulq16_integer(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 2.0 × 3.0 = 6.0 */
    int32_t a = 2 << 16;
    int32_t b = 3 << 16;
    int32_t result = dvm_mul_q16(a, b, &faults);
    
    if (result != (6 << 16)) return 0;
    if (faults.overflow != 0) return 0;
    
    return 1;
}

static int test_mulq16_fractional(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 0.5 × 0.5 = 0.25 */
    int32_t a = FIXED_HALF;
    int32_t b = FIXED_HALF;
    int32_t result = dvm_mul_q16(a, b, &faults);
    
    /* 0.25 in Q16.16 = 16384 */
    return result == (1 << 14);
}

static int test_mulq16_zero(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_mul_q16(FIXED_ONE, 0, &faults);
    
    return result == 0;
}

static int test_mulq16_one(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t x = 12345;
    int32_t result = dvm_mul_q16(x, FIXED_ONE, &faults);
    
    /* Multiplying by 1.0 should give same value (within rounding) */
    return result == x;
}

static int test_mulq16_negative(void)
{
    ct_fault_flags_t faults = {0};
    
    /* -2.0 × 3.0 = -6.0 */
    int32_t a = -(2 << 16);
    int32_t b = 3 << 16;
    int32_t result = dvm_mul_q16(a, b, &faults);
    
    return result == -(6 << 16);
}

/* ============================================================================
 * Test: Q16.16 Fixed-Point Division
 * ============================================================================ */

static int test_divq16_integer(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 6.0 ÷ 2.0 = 3.0 */
    int32_t num = 6 << 16;
    int32_t denom = 2 << 16;
    int32_t result = dvm_div_q16(num, denom, &faults);
    
    if (result != (3 << 16)) return 0;
    if (faults.div_zero != 0) return 0;
    
    return 1;
}

static int test_divq16_fractional(void)
{
    ct_fault_flags_t faults = {0};
    
    /* 1.0 ÷ 2.0 = 0.5 */
    int32_t result = dvm_div_q16(FIXED_ONE, 2 << 16, &faults);
    
    return result == FIXED_HALF;
}

static int test_divq16_by_zero(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t result = dvm_div_q16(FIXED_ONE, 0, &faults);
    
    if (result != 0) return 0;
    if (faults.div_zero != 1) return 0;
    
    return 1;
}

static int test_divq16_by_one(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t x = 12345 << 16;
    int32_t result = dvm_div_q16(x, FIXED_ONE, &faults);
    
    return result == x;
}

/* ============================================================================
 * Test: Fault Flag Management
 * ============================================================================ */

static int test_fault_clear(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Set all flags */
    faults.overflow = 1;
    faults.underflow = 1;
    faults.div_zero = 1;
    faults.domain = 1;
    faults.precision = 1;
    faults.grad_floor = 1;
    faults.chain_invalid = 1;
    
    /* Clear */
    ct_fault_clear(&faults);
    
    if (faults.overflow != 0) return 0;
    if (faults.underflow != 0) return 0;
    if (faults.div_zero != 0) return 0;
    if (faults.domain != 0) return 0;
    if (faults.precision != 0) return 0;
    if (faults.grad_floor != 0) return 0;
    if (faults.chain_invalid != 0) return 0;
    
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - DVM Primitives Tests\n");
    printf("Traceability: SRS-001-LOADER, CT-MATH-001 §3\n");
    printf("==============================================\n\n");
    
    printf("Clamp32:\n");
    RUN_TEST(test_clamp32_no_overflow);
    RUN_TEST(test_clamp32_overflow);
    RUN_TEST(test_clamp32_underflow);
    RUN_TEST(test_clamp32_boundary_max);
    RUN_TEST(test_clamp32_boundary_min);
    
    printf("\nSaturating Addition:\n");
    RUN_TEST(test_add32_normal);
    RUN_TEST(test_add32_overflow);
    RUN_TEST(test_add32_underflow);
    RUN_TEST(test_add32_negative);
    RUN_TEST(test_add32_mixed_sign);
    
    printf("\nSaturating Subtraction:\n");
    RUN_TEST(test_sub32_normal);
    RUN_TEST(test_sub32_overflow);
    RUN_TEST(test_sub32_underflow);
    
    printf("\n64-bit Multiply:\n");
    RUN_TEST(test_mul64_positive);
    RUN_TEST(test_mul64_negative);
    RUN_TEST(test_mul64_zero);
    RUN_TEST(test_mul64_max);
    
    printf("\nRound-to-Nearest-Even (CT-MATH-001 test vectors):\n");
    RUN_TEST(test_rne_test_vector_1);
    RUN_TEST(test_rne_test_vector_2);
    RUN_TEST(test_rne_test_vector_3);
    RUN_TEST(test_rne_negative);
    RUN_TEST(test_rne_round_down);
    RUN_TEST(test_rne_round_up);
    RUN_TEST(test_rne_shift_zero);
    RUN_TEST(test_rne_shift_bounds);
    
    printf("\nQ16.16 Multiply:\n");
    RUN_TEST(test_mulq16_integer);
    RUN_TEST(test_mulq16_fractional);
    RUN_TEST(test_mulq16_zero);
    RUN_TEST(test_mulq16_one);
    RUN_TEST(test_mulq16_negative);
    
    printf("\nQ16.16 Division:\n");
    RUN_TEST(test_divq16_integer);
    RUN_TEST(test_divq16_fractional);
    RUN_TEST(test_divq16_by_zero);
    RUN_TEST(test_divq16_by_one);
    
    printf("\nFault Flags:\n");
    RUN_TEST(test_fault_clear);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
