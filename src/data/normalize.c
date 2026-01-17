/**
 * @file normalize.c
 * @project Certifiable Data Pipeline
 * @brief Fixed-point data normalization.
 *
 * @details Normalizes data to zero mean, unit variance using precomputed
 *          statistics. All operations in Q16.16 fixed-point.
 *
 * @traceability SRS-002-NORMALIZE, CT-MATH-001 §4
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "normalize.h"
#include "dvm.h"
#include <string.h>

/*===========================================================================*/
/* ct_normalize_init                                                          */
/*===========================================================================*/

void ct_normalize_init(ct_normalize_ctx_t *ctx,
                       const int32_t *means,
                       const int32_t *inv_stds,
                       uint32_t num_features)
{
    ctx->means = means;
    ctx->inv_stds = inv_stds;
    ctx->num_features = num_features;
}

/*===========================================================================*/
/* ct_normalize_sample (CT-MATH-001 §4.2)                                    */
/*===========================================================================*/

void ct_normalize_sample(const ct_normalize_ctx_t *ctx,
                         const ct_sample_t *input,
                         ct_sample_t *output,
                         ct_fault_flags_t *faults)
{
    /* Copy metadata */
    output->version = input->version;
    output->dtype = input->dtype;
    output->ndims = input->ndims;
    for (uint32_t i = 0; i < CT_MAX_DIMS; i++) {
        output->dims[i] = input->dims[i];
    }
    output->total_elements = input->total_elements;
    
    /* Normalize each element: y = (x - mean) * inv_std */
    for (uint32_t i = 0; i < input->total_elements && i < ctx->num_features; i++) {
        int32_t x = input->data[i];
        int32_t mean = ctx->means[i];
        int32_t inv_std = ctx->inv_stds[i];
        
        /* Subtract mean (saturating) */
        int32_t centered = dvm_sub32(x, mean, faults);
        
        /* Multiply by inverse std (Q16.16 × Q16.16 → Q16.16) */
        int32_t normalized = dvm_mul_q16(centered, inv_std, faults);
        
        output->data[i] = normalized;
    }
    
    /* Copy remaining elements unchanged */
    for (uint32_t i = ctx->num_features; i < input->total_elements; i++) {
        output->data[i] = input->data[i];
    }
}

/*===========================================================================*/
/* ct_normalize_batch                                                         */
/*===========================================================================*/

void ct_normalize_batch(const ct_normalize_ctx_t *ctx,
                        const ct_batch_t *input,
                        ct_batch_t *output,
                        ct_fault_flags_t *faults)
{
    for (uint32_t i = 0; i < input->batch_size; i++) {
        ct_normalize_sample(ctx, &input->samples[i], &output->samples[i], faults);
    }
    
    output->batch_size = input->batch_size;
    output->batch_index = input->batch_index;
    memcpy(output->batch_hash, input->batch_hash, 32);
}
