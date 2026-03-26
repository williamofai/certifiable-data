/**
 * @file prng.h
 * @project Certifiable Data Pipeline
 * @brief Deterministic PRNG interface.
 *
 * @traceability CT-MATH-001 §5, CT-STRUCT-001 §7
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_PRNG_H
#define CT_PRNG_H

#include <stdint.h>

/**
 * @brief Generate deterministic pseudo-random 64-bit value.
 * @param seed Global random seed
 * @param epoch Current epoch
 * @param op_id Operation identifier (unique per call site)
 * @return 64-bit pseudo-random value
 * @traceability CT-MATH-001 §5.1
 */
uint64_t ct_prng(uint64_t seed, uint32_t epoch, uint32_t op_id);

/**
 * @brief Generate uniform random integer in [0, n).
 * @param seed Global random seed
 * @param epoch Current epoch
 * @param op_id Operation identifier
 * @param n Upper bound (exclusive)
 * @return Random value in [0, n)
 * @traceability CT-MATH-001 §5.3
 */
uint32_t ct_prng_uniform(uint64_t seed, uint32_t epoch, uint32_t op_id, uint32_t n);

#endif /* CT_PRNG_H */
