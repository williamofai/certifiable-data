/**
 * @file shuffle.h
 * @project Certifiable Data Pipeline
 * @brief Deterministic shuffling interface.
 *
 * @traceability SRS-004-SHUFFLE, CT-STRUCT-001 §9
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_SHUFFLE_H
#define CT_SHUFFLE_H

#include "ct_types.h"

/**
 * @brief Permute index using Feistel network (bijective).
 * @param index Input index [0, N)
 * @param N Dataset size
 * @param seed Random seed
 * @param epoch Current epoch
 * @return Permuted index [0, N)
 * @traceability CT-MATH-001 §7.2, REQ-SHUF-001
 */
uint32_t ct_permute_index(uint32_t index, uint32_t N, uint64_t seed, uint32_t epoch);

/**
 * @brief Initialize shuffle context.
 * @param ctx Shuffle context
 * @param seed Random seed
 * @param epoch Current epoch
 * @traceability REQ-SHUF-002
 */
void ct_shuffle_init(ct_shuffle_ctx_t *ctx, uint64_t seed, uint32_t epoch);

/**
 * @brief Verify Feistel permutation is bijective (sanity check).
 * @param seed Random seed
 * @param epoch Current epoch
 * @param N Dataset size
 * @param num_samples Number of samples to check
 * @return 1 if valid, 0 if invalid
 * @traceability REQ-SHUF-003
 */
int ct_shuffle_verify_bijection(uint64_t seed, uint32_t epoch, uint32_t N, uint32_t num_samples);

#endif /* CT_SHUFFLE_H */
