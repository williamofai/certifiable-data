/**
 * @file batch.h
 * @project Certifiable Data Pipeline
 * @brief Batch construction interface.
 *
 * @traceability SRS-005-BATCH, CT-STRUCT-001 §10
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_BATCH_H
#define CT_BATCH_H

#include "ct_types.h"

/**
 * @brief Initialize batch structure.
 * @param batch Batch to initialize
 * @param samples Pre-allocated sample array
 * @param sample_hashes Pre-allocated hash array
 * @param batch_size Maximum samples per batch
 * @traceability REQ-BATCH-001
 */
void ct_batch_init(ct_batch_t *batch,
                   ct_sample_t *samples,
                   ct_hash_t *sample_hashes,
                   uint32_t batch_size);

/**
 * @brief Fill batch with shuffled samples from dataset.
 * @param batch Batch to fill
 * @param dataset Source dataset
 * @param batch_index Index of this batch
 * @param epoch Current epoch
 * @param seed Random seed
 * @traceability REQ-BATCH-002, REQ-BATCH-003, CT-MATH-001 §9.1
 */
void ct_batch_fill(ct_batch_t *batch,
                   const ct_dataset_t *dataset,
                   uint32_t batch_index,
                   uint32_t epoch,
                   uint64_t seed);

/**
 * @brief Get sample from batch.
 * @param batch Source batch
 * @param index Sample index within batch
 * @return Pointer to sample, or NULL if invalid index
 * @traceability REQ-BATCH-004
 */
const ct_sample_t* ct_batch_get_sample(const ct_batch_t *batch, uint32_t index);

/**
 * @brief Verify batch hash.
 * @param batch Batch to verify
 * @return 1 if valid, 0 if invalid
 * @traceability REQ-BATCH-005
 */
int ct_batch_verify(const ct_batch_t *batch);

#endif /* CT_BATCH_H */
