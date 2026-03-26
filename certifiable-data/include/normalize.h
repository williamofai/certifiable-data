/**
 * @file normalize.h
 * @project Certifiable Data Pipeline
 * @brief Data normalization interface.
 *
 * @traceability SRS-002-NORMALIZE, CT-STRUCT-001 §6
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_NORMALIZE_H
#define CT_NORMALIZE_H

#include "ct_types.h"

/**
 * @brief Initialize normalization context.
 * @param ctx Normalization context
 * @param means Array of mean values (Q16.16)
 * @param inv_stds Array of inverse standard deviations (Q16.16)
 * @param num_features Number of features
 * @traceability REQ-NORM-001
 */
void ct_normalize_init(ct_normalize_ctx_t *ctx,
                       const int32_t *means,
                       const int32_t *inv_stds,
                       uint32_t num_features);

/**
 * @brief Normalize single sample.
 * @param ctx Normalization context
 * @param input Input sample
 * @param output Output sample (normalized)
 * @param faults Fault flags
 * @traceability REQ-NORM-002, CT-MATH-001 §4.2
 */
void ct_normalize_sample(const ct_normalize_ctx_t *ctx,
                         const ct_sample_t *input,
                         ct_sample_t *output,
                         ct_fault_flags_t *faults);

/**
 * @brief Normalize entire batch.
 * @param ctx Normalization context
 * @param input Input batch
 * @param output Output batch (normalized)
 * @param faults Fault flags
 * @traceability REQ-NORM-003
 */
void ct_normalize_batch(const ct_normalize_ctx_t *ctx,
                        const ct_batch_t *input,
                        ct_batch_t *output,
                        ct_fault_flags_t *faults);

#endif /* CT_NORMALIZE_H */
