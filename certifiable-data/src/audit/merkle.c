/**
 * @file merkle.c
 * @project Certifiable Data Pipeline
 * @brief Merkle tree construction and provenance chain management.
 *
 * @details Computes sample hashes, batch Merkle roots, epoch hashes,
 *          and maintains the dataset provenance chain.
 *
 * @traceability SRS-006-MERKLE, CT-MATH-001 §10
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "merkle.h"
#include "sha256.h"
#include <string.h>

/*===========================================================================*/
/* ct_hash_sample (CT-MATH-001 §10.1)                                        */
/*===========================================================================*/

void ct_hash_sample(const ct_sample_t *sample, ct_hash_t out_hash)
{
    ct_sha256_ctx_t ctx;
    ct_sha256_init(&ctx);
    
    /* Domain separator for leaf */
    uint8_t prefix = CT_DOMAIN_LEAF;
    ct_sha256_update(&ctx, &prefix, 1);
    
    /* Serialize sample header */
    uint8_t header[32];
    uint32_t offset = 0;
    
    /* version (4 bytes, little-endian) */
    header[offset++] = (uint8_t)(sample->version & 0xFF);
    header[offset++] = (uint8_t)((sample->version >> 8) & 0xFF);
    header[offset++] = (uint8_t)((sample->version >> 16) & 0xFF);
    header[offset++] = (uint8_t)((sample->version >> 24) & 0xFF);
    
    /* dtype (4 bytes) */
    header[offset++] = (uint8_t)(sample->dtype & 0xFF);
    header[offset++] = (uint8_t)((sample->dtype >> 8) & 0xFF);
    header[offset++] = (uint8_t)((sample->dtype >> 16) & 0xFF);
    header[offset++] = (uint8_t)((sample->dtype >> 24) & 0xFF);
    
    /* ndims (4 bytes) */
    header[offset++] = (uint8_t)(sample->ndims & 0xFF);
    header[offset++] = (uint8_t)((sample->ndims >> 8) & 0xFF);
    header[offset++] = (uint8_t)((sample->ndims >> 16) & 0xFF);
    header[offset++] = (uint8_t)((sample->ndims >> 24) & 0xFF);
    
    /* dims (up to 4 dimensions, 4 bytes each) */
    for (uint32_t i = 0; i < CT_MAX_DIMS; i++) {
        uint32_t dim = (i < sample->ndims) ? sample->dims[i] : 0;
        header[offset++] = (uint8_t)(dim & 0xFF);
        header[offset++] = (uint8_t)((dim >> 8) & 0xFF);
        header[offset++] = (uint8_t)((dim >> 16) & 0xFF);
        header[offset++] = (uint8_t)((dim >> 24) & 0xFF);
    }
    
    ct_sha256_update(&ctx, header, offset);
    
    /* Hash data elements (little-endian int32_t) */
    for (uint32_t i = 0; i < sample->total_elements; i++) {
        uint8_t buf[4];
        int32_t val = sample->data[i];
        buf[0] = (uint8_t)(val & 0xFF);
        buf[1] = (uint8_t)((val >> 8) & 0xFF);
        buf[2] = (uint8_t)((val >> 16) & 0xFF);
        buf[3] = (uint8_t)((val >> 24) & 0xFF);
        ct_sha256_update(&ctx, buf, 4);
    }
    
    ct_sha256_final(&ctx, out_hash);
}

/*===========================================================================*/
/* ct_hash_internal (CT-MATH-001 §10.2)                                      */
/*===========================================================================*/

void ct_hash_internal(const ct_hash_t left, const ct_hash_t right, ct_hash_t out_hash)
{
    ct_sha256_ctx_t ctx;
    ct_sha256_init(&ctx);
    
    /* Domain separator for internal node */
    uint8_t prefix = CT_DOMAIN_INTERNAL;
    ct_sha256_update(&ctx, &prefix, 1);
    
    ct_sha256_update(&ctx, left, 32);
    ct_sha256_update(&ctx, right, 32);
    
    ct_sha256_final(&ctx, out_hash);
}

/*===========================================================================*/
/* ct_merkle_root (CT-MATH-001 §10.3)                                        */
/*===========================================================================*/

void ct_merkle_root(const ct_hash_t *leaves, uint32_t count, ct_hash_t out_root)
{
    if (count == 0) {
        memset(out_root, 0, 32);
        return;
    }
    
    if (count == 1) {
        memcpy(out_root, leaves[0], 32);
        return;
    }
    
    /* Build tree bottom-up using temporary storage */
    /* Maximum tree depth for 2^20 samples is 20 levels */
    ct_hash_t tree[2048];  /* Support up to 1024 leaves in this pass */
    
    uint32_t current_level = count;
    uint32_t next_level;
    
    /* Copy leaves to tree */
    for (uint32_t i = 0; i < count && i < 1024; i++) {
        memcpy(tree[i], leaves[i], 32);
    }
    
    /* Build tree upward */
    while (current_level > 1) {
        next_level = (current_level + 1) / 2;
        
        for (uint32_t i = 0; i < next_level; i++) {
            uint32_t left_idx = i * 2;
            uint32_t right_idx = left_idx + 1;
            
            if (right_idx < current_level) {
                /* Both children exist */
                ct_hash_internal(tree[left_idx], tree[right_idx], tree[i]);
            } else {
                /* Odd number of nodes - promote last node */
                memcpy(tree[i], tree[left_idx], 32);
            }
        }
        
        current_level = next_level;
    }
    
    memcpy(out_root, tree[0], 32);
}

/*===========================================================================*/
/* ct_hash_batch (CT-MATH-001 §10.4)                                         */
/*===========================================================================*/

void ct_hash_batch(const ct_batch_t *batch, ct_hash_t out_hash)
{
    /* Compute Merkle root over sample hashes */
    ct_merkle_root((const ct_hash_t *)batch->sample_hashes, batch->batch_size, out_hash);
}

/*===========================================================================*/
/* ct_hash_epoch (CT-MATH-001 §10.5)                                         */
/*===========================================================================*/

void ct_hash_epoch(const ct_hash_t *batch_hashes, uint32_t num_batches, ct_hash_t out_hash)
{
    ct_merkle_root(batch_hashes, num_batches, out_hash);
}

/*===========================================================================*/
/* ct_provenance_init (CT-MATH-001 §10.6)                                    */
/*===========================================================================*/

void ct_provenance_init(ct_provenance_t *prov,
                        const ct_hash_t dataset_hash,
                        const ct_hash_t config_hash,
                        uint64_t seed)
{
    prov->current_epoch = 0;
    prov->total_epochs = 0;
    
    /* h_0 = SHA256(0x03 || H_dataset || H_config || seed) */
    ct_sha256_ctx_t ctx;
    ct_sha256_init(&ctx);
    
    uint8_t prefix = CT_DOMAIN_PROVENANCE;
    ct_sha256_update(&ctx, &prefix, 1);
    
    ct_sha256_update(&ctx, dataset_hash, 32);
    ct_sha256_update(&ctx, config_hash, 32);
    
    /* seed (little-endian) */
    uint8_t buf[8];
    buf[0] = (uint8_t)(seed & 0xFF);
    buf[1] = (uint8_t)((seed >> 8) & 0xFF);
    buf[2] = (uint8_t)((seed >> 16) & 0xFF);
    buf[3] = (uint8_t)((seed >> 24) & 0xFF);
    buf[4] = (uint8_t)((seed >> 32) & 0xFF);
    buf[5] = (uint8_t)((seed >> 40) & 0xFF);
    buf[6] = (uint8_t)((seed >> 48) & 0xFF);
    buf[7] = (uint8_t)((seed >> 56) & 0xFF);
    ct_sha256_update(&ctx, buf, 8);
    
    ct_sha256_final(&ctx, prov->current_hash);
    
    /* prev_hash = current_hash initially */
    memcpy(prov->prev_hash, prov->current_hash, 32);
}

/*===========================================================================*/
/* ct_provenance_advance (CT-MATH-001 §10.7)                                 */
/*===========================================================================*/

void ct_provenance_advance(ct_provenance_t *prov, const ct_hash_t epoch_hash)
{
    /* Save current as previous */
    memcpy(prov->prev_hash, prov->current_hash, 32);
    
    /* h_e = SHA256(0x04 || h_{e-1} || H_epoch || e) */
    ct_sha256_ctx_t ctx;
    ct_sha256_init(&ctx);
    
    uint8_t prefix = CT_DOMAIN_EPOCH_CHAIN;
    ct_sha256_update(&ctx, &prefix, 1);
    
    ct_sha256_update(&ctx, prov->prev_hash, 32);
    ct_sha256_update(&ctx, epoch_hash, 32);
    
    /* epoch (little-endian) */
    uint32_t epoch = prov->current_epoch;
    uint8_t buf[4];
    buf[0] = (uint8_t)(epoch & 0xFF);
    buf[1] = (uint8_t)((epoch >> 8) & 0xFF);
    buf[2] = (uint8_t)((epoch >> 16) & 0xFF);
    buf[3] = (uint8_t)((epoch >> 24) & 0xFF);
    ct_sha256_update(&ctx, buf, 4);
    
    ct_sha256_final(&ctx, prov->current_hash);
    
    prov->current_epoch++;
    prov->total_epochs++;
}
