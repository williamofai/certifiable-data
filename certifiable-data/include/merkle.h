/**
 * @file merkle.h
 * @project Certifiable Data Pipeline
 * @brief Merkle tree and provenance chain interface.
 *
 * @traceability SRS-006-MERKLE, CT-STRUCT-001 §4,§12
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_MERKLE_H
#define CT_MERKLE_H

#include "ct_types.h"

/**
 * @brief Compute hash of a single sample (leaf node).
 * @param sample Sample to hash
 * @param out_hash Output hash (32 bytes)
 * @traceability REQ-MERK-001, CT-MATH-001 §10.1
 */
void ct_hash_sample(const ct_sample_t *sample, ct_hash_t out_hash);

/**
 * @brief Compute hash of internal Merkle node.
 * @param left Left child hash
 * @param right Right child hash
 * @param out_hash Output hash (32 bytes)
 * @traceability REQ-MERK-002, CT-MATH-001 §10.2
 */
void ct_hash_internal(const ct_hash_t left, const ct_hash_t right, ct_hash_t out_hash);

/**
 * @brief Compute Merkle root from array of leaf hashes.
 * @param leaves Array of leaf hashes
 * @param count Number of leaves
 * @param out_root Output root hash
 * @traceability REQ-MERK-003, CT-MATH-001 §10.3
 */
void ct_merkle_root(const ct_hash_t *leaves, uint32_t count, ct_hash_t out_root);

/**
 * @brief Compute batch hash (Merkle root of samples).
 * @param batch Batch to hash
 * @param out_hash Output hash
 * @traceability REQ-MERK-004, CT-MATH-001 §10.4
 */
void ct_hash_batch(const ct_batch_t *batch, ct_hash_t out_hash);

/**
 * @brief Compute epoch hash (Merkle root of batches).
 * @param batch_hashes Array of batch hashes
 * @param num_batches Number of batches
 * @param out_hash Output hash
 * @traceability REQ-MERK-005, CT-MATH-001 §10.5
 */
void ct_hash_epoch(const ct_hash_t *batch_hashes, uint32_t num_batches, ct_hash_t out_hash);

/**
 * @brief Initialize provenance chain.
 * @param prov Provenance structure
 * @param dataset_hash Hash of dataset
 * @param config_hash Hash of configuration
 * @param seed Random seed
 * @traceability REQ-MERK-006, CT-MATH-001 §10.6
 */
void ct_provenance_init(ct_provenance_t *prov,
                        const ct_hash_t dataset_hash,
                        const ct_hash_t config_hash,
                        uint64_t seed);

/**
 * @brief Advance provenance chain to next epoch.
 * @param prov Provenance structure (updated in place)
 * @param epoch_hash Hash of completed epoch
 * @traceability REQ-MERK-007, CT-MATH-001 §10.7
 */
void ct_provenance_advance(ct_provenance_t *prov, const ct_hash_t epoch_hash);

#endif /* CT_MERKLE_H */
