/**
 * @file primitives.c
 * @project Certifiable Data Pipeline
 * @brief DVM (Deterministic Virtual Machine) arithmetic primitives.
 *
 * @details Core integer-only operations with explicit overflow handling.
 *          All operations are bit-identical across platforms.
 *
 * @traceability CT-MATH-001 §3, SRS-001-LOADER
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "dvm.h"
#include <stdint.h>

/*===========================================================================*/
/* DVM_Clamp32 (CT-MATH-001 §3.1)                                            */
/*===========================================================================*/

int32_t dvm_clamp32(int64_t x, ct_fault_flags_t *faults)
{
    if (x > INT32_MAX) {
        faults->overflow = 1;
        return INT32_MAX;
    }
    if (x < INT32_MIN) {
        faults->underflow = 1;
        return INT32_MIN;
    }
    return (int32_t)x;
}

/*===========================================================================*/
/* DVM_Add32 (CT-MATH-001 §3.2)                                              */
/*===========================================================================*/

int32_t dvm_add32(int32_t a, int32_t b, ct_fault_flags_t *faults)
{
    int64_t wide = (int64_t)a + (int64_t)b;
    return dvm_clamp32(wide, faults);
}

/*===========================================================================*/
/* DVM_Sub32 (CT-MATH-001 §3.3)                                              */
/*===========================================================================*/

int32_t dvm_sub32(int32_t a, int32_t b, ct_fault_flags_t *faults)
{
    int64_t wide = (int64_t)a - (int64_t)b;
    return dvm_clamp32(wide, faults);
}

/*===========================================================================*/
/* DVM_Mul64 (CT-MATH-001 §3.4)                                              */
/*===========================================================================*/

int64_t dvm_mul64(int32_t a, int32_t b)
{
    return (int64_t)a * (int64_t)b;
}

/*===========================================================================*/
/* DVM_RoundShiftR_RNE (CT-MATH-001 §3.5)                                    */
/*===========================================================================*/

int32_t dvm_round_shift_rne(int64_t x, uint32_t shift, ct_fault_flags_t *faults)
{
    if (shift > 62) {
        faults->domain = 1;
        return 0;
    }
    
    if (shift == 0) {
        return dvm_clamp32(x, faults);
    }
    
    /* Extract fractional part and quotient */
    int64_t mask = (1LL << shift) - 1;
    int64_t halfway = 1LL << (shift - 1);
    int64_t frac = x & mask;
    int64_t quot = x >> shift;  /* Arithmetic shift preserves sign */
    
    int64_t result;
    if (frac < halfway) {
        /* Round down */
        result = quot;
    } else if (frac > halfway) {
        /* Round up */
        result = quot + 1;
    } else {
        /* Exactly halfway - round to even */
        result = quot + (quot & 1);
    }
    
    return dvm_clamp32(result, faults);
}

/*===========================================================================*/
/* DVM_Mul_Q16 (CT-MATH-001 §3.6)                                            */
/*===========================================================================*/

int32_t dvm_mul_q16(int32_t a, int32_t b, ct_fault_flags_t *faults)
{
    int64_t prod = dvm_mul64(a, b);
    return dvm_round_shift_rne(prod, 16, faults);
}

/*===========================================================================*/
/* DVM_Div_Q16 (CT-MATH-001 §3.7)                                            */
/*===========================================================================*/

int32_t dvm_div_q16(int32_t num, int32_t denom, ct_fault_flags_t *faults)
{
    if (denom == 0) {
        faults->div_zero = 1;
        return 0;
    }
    
    /* Scale numerator to Q32.16, then divide */
    int64_t num_scaled = ((int64_t)num) << 16;
    int64_t result = num_scaled / denom;
    
    return dvm_clamp32(result, faults);
}

/*===========================================================================*/
/* Fault flag helpers                                                         */
/*===========================================================================*/

void ct_fault_clear(ct_fault_flags_t *faults)
{
    faults->overflow = 0;
    faults->underflow = 0;
    faults->div_zero = 0;
    faults->domain = 0;
    faults->precision = 0;
    faults->grad_floor = 0;
    faults->chain_invalid = 0;
}

int ct_has_fault(const ct_fault_flags_t *faults)
{
    return faults->overflow || faults->underflow || faults->div_zero || 
           faults->domain || faults->precision || faults->grad_floor ||
           faults->chain_invalid;
}
