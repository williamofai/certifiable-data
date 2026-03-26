/**
 * @file batch.c
 * @project Certifiable Data Pipeline
 * @brief Batch construction with Merkle commitment.
 *
 * @details Constructs batches from shuffled dataset with cryptographic
 *          commitment to batch contents.
 *
 * @traceability SRS-005-BATCH, CT-MATH-001 §9
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "batch.h"
#include "shuffle.h"
#include "merkle.h"
#include <string.h>

/*===========================================================================*/
/* ct_batch_init                                                              */
/*===========================================================================*/

void ct_batch_init(ct_batch_t *batch,
                   ct_sample_t *samples,
                   ct_hash_t *sample_hashes,
                   uint32_t batch_size)
{
    batch->samples = samples;
    batch->sample_hashes = sample_hashes;
    batch->batch_size = batch_size;
    batch->batch_index = 0;
    memset(batch->batch_hash, 0, 32);
}

/*===========================================================================*/
/* ct_batch_fill (CT-MATH-001 §9.1)                                          */
/*===========================================================================*/

void ct_batch_fill(ct_batch_t *batch,
                   const ct_dataset_t *dataset,
                   uint32_t batch_index,
                   uint32_t epoch,
                   uint64_t seed)
{
    batch->batch_index = batch_index;
    
    uint32_t start_idx = batch_index * batch->batch_size;
    uint32_t samples_in_batch = batch->batch_size;
    
    /* Last batch may be partial */
    if (start_idx + samples_in_batch > dataset->num_samples) {
        samples_in_batch = dataset->num_samples - start_idx;
    }
    
    /* Fill batch with shuffled samples */
    for (uint32_t i = 0; i < samples_in_batch; i++) {
        uint32_t global_idx = start_idx + i;
        uint32_t shuffled_idx = ct_permute_index(global_idx,
                                                  dataset->num_samples,
                                                  seed,
                                                  epoch);
        
        /* Copy sample (shallow copy - data pointer remains) */
        batch->samples[i] = dataset->samples[shuffled_idx];
        
        /* Compute and store sample hash */
        ct_hash_sample(&batch->samples[i], batch->sample_hashes[i]);
    }
    
    /* Pad remaining slots with zeros if partial batch */
    for (uint32_t i = samples_in_batch; i < batch->batch_size; i++) {
        memset(&batch->samples[i], 0, sizeof(ct_sample_t));
        memset(batch->sample_hashes[i], 0, 32);
    }
    
    /* Compute batch Merkle root */
    ct_hash_batch(batch, batch->batch_hash);
}

/*===========================================================================*/
/* ct_batch_get_sample                                                        */
/*===========================================================================*/

const ct_sample_t* ct_batch_get_sample(const ct_batch_t *batch, uint32_t index)
{
    if (index >= batch->batch_size) {
        return NULL;
    }
    return &batch->samples[index];
}

/*===========================================================================*/
/* ct_batch_verify                                                            */
/*===========================================================================*/

int ct_batch_verify(const ct_batch_t *batch)
{
    ct_hash_t computed_hash;
    ct_hash_batch(batch, computed_hash);
    
    return (memcmp(computed_hash, batch->batch_hash, 32) == 0) ? 1 : 0;
}
