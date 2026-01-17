/**
 * @file dvm.h
 * @project Certifiable Data Pipeline
 * @brief DVM (Deterministic Virtual Machine) primitives interface.
 *
 * @details Core arithmetic operations for deterministic fixed-point computation.
 *
 * @traceability CT-MATH-001 §3, CT-STRUCT-001 §2
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_DVM_H
#define CT_DVM_H

#include "ct_types.h"
#include <stdint.h>

/*===========================================================================*/
/* DVM Primitives                                                             */
/*===========================================================================*/

/**
 * @brief Clamp 64-bit value to 32-bit range with fault signaling.
 * @param x Value to clamp
 * @param faults Fault flags (updated if clamped)
 * @return Clamped 32-bit value
 * @traceability CT-MATH-001 §3.1
 */
int32_t dvm_clamp32(int64_t x, ct_fault_flags_t *faults);

/**
 * @brief Saturating 32-bit addition.
 * @param a First operand
 * @param b Second operand
 * @param faults Fault flags (updated on overflow)
 * @return a + b (saturated)
 * @traceability CT-MATH-001 §3.2
 */
int32_t dvm_add32(int32_t a, int32_t b, ct_fault_flags_t *faults);

/**
 * @brief Saturating 32-bit subtraction.
 * @param a First operand
 * @param b Second operand
 * @param faults Fault flags (updated on overflow)
 * @return a - b (saturated)
 * @traceability CT-MATH-001 §3.3
 */
int32_t dvm_sub32(int32_t a, int32_t b, ct_fault_flags_t *faults);

/**
 * @brief 32×32→64 multiply (no overflow).
 * @param a First operand
 * @param b Second operand
 * @return a × b (64-bit result)
 * @traceability CT-MATH-001 §3.4
 */
int64_t dvm_mul64(int32_t a, int32_t b);

/**
 * @brief Round-to-nearest-even right shift.
 * @param x Value to shift
 * @param shift Shift amount (0-62)
 * @param faults Fault flags (domain error if shift > 62)
 * @return x >> shift (rounded)
 * @traceability CT-MATH-001 §3.5
 */
int32_t dvm_round_shift_rne(int64_t x, uint32_t shift, ct_fault_flags_t *faults);

/**
 * @brief Q16.16 fixed-point multiplication.
 * @param a First operand (Q16.16)
 * @param b Second operand (Q16.16)
 * @param faults Fault flags
 * @return a × b (Q16.16)
 * @traceability CT-MATH-001 §3.6
 */
int32_t dvm_mul_q16(int32_t a, int32_t b, ct_fault_flags_t *faults);

/**
 * @brief Q16.16 fixed-point division.
 * @param num Numerator (Q16.16)
 * @param denom Denominator (Q16.16)
 * @param faults Fault flags (div_zero if denom == 0)
 * @return num / denom (Q16.16)
 * @traceability CT-MATH-001 §3.7
 */
int32_t dvm_div_q16(int32_t num, int32_t denom, ct_fault_flags_t *faults);

/*===========================================================================*/
/* Fault flag helpers                                                         */
/*===========================================================================*/

/**
 * @brief Clear all fault flags.
 * @param faults Fault flags structure
 */
void ct_fault_clear(ct_fault_flags_t *faults);

#endif /* CT_DVM_H */
