/**
 * @file augment.h
 * @project Certifiable Data Pipeline
 * @brief Data augmentation interface.
 *
 * @traceability SRS-003-AUGMENT, CT-STRUCT-001 §8
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_AUGMENT_H
#define CT_AUGMENT_H

#include "ct_types.h"

/**
 * @brief Initialize augmentation context.
 * @param ctx Augmentation context
 * @param seed Random seed
 * @param epoch Current epoch
 * @param flags Augmentation flags (which transforms to apply)
 * @traceability REQ-AUG-001
 */
void ct_augment_init(ct_augment_ctx_t *ctx,
                     uint64_t seed,
                     uint32_t epoch,
                     ct_augment_flags_t flags);

/**
 * @brief Augment single sample.
 * @param ctx Augmentation context
 * @param input Input sample
 * @param output Output sample (augmented)
 * @param sample_idx Global sample index (for PRNG)
 * @param faults Fault flags
 * @traceability REQ-AUG-002, CT-MATH-001 §6
 */
void ct_augment_sample(const ct_augment_ctx_t *ctx,
                       const ct_sample_t *input,
                       ct_sample_t *output,
                       uint32_t sample_idx,
                       ct_fault_flags_t *faults);

/**
 * @brief Augment entire batch.
 * @param ctx Augmentation context
 * @param input Input batch
 * @param output Output batch (augmented)
 * @param faults Fault flags
 * @traceability REQ-AUG-003
 */
void ct_augment_batch(const ct_augment_ctx_t *ctx,
                      const ct_batch_t *input,
                      ct_batch_t *output,
                      ct_fault_flags_t *faults);

#endif /* CT_AUGMENT_H */
