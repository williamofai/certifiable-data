/**
 * @file augment.c
 * @project Certifiable Data Pipeline
 * @brief Deterministic data augmentation.
 *
 * @details Applies deterministic transformations (flip, crop, noise) using PRNG.
 *
 * @traceability SRS-003-AUGMENT, CT-MATH-001 §6
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "augment.h"
#include "prng.h"
#include "dvm.h"
#include <string.h>

/*===========================================================================*/
/* ct_augment_init                                                            */
/*===========================================================================*/

void ct_augment_init(ct_augment_ctx_t *ctx,
                     uint64_t seed,
                     uint32_t epoch,
                     ct_augment_flags_t flags)
{
    ctx->seed = seed;
    ctx->epoch = epoch;
    ctx->flags = flags;
}

/*===========================================================================*/
/* ct_augment_horizontal_flip (CT-MATH-001 §6.1)                             */
/*===========================================================================*/

static void horizontal_flip(ct_sample_t *sample, uint32_t width, uint32_t height)
{
    /* Flip horizontally: swap columns */
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width / 2; col++) {
            uint32_t left_idx = row * width + col;
            uint32_t right_idx = row * width + (width - 1 - col);
            
            int32_t temp = sample->data[left_idx];
            sample->data[left_idx] = sample->data[right_idx];
            sample->data[right_idx] = temp;
        }
    }
}

/*===========================================================================*/
/* ct_augment_random_crop (CT-MATH-001 §6.2)                                 */
/*===========================================================================*/

static void random_crop(ct_sample_t *input,
                        ct_sample_t *output,
                        uint32_t src_width,
                        uint32_t src_height,
                        uint32_t crop_width,
                        uint32_t crop_height,
                        uint64_t seed,
                        uint32_t epoch,
                        uint32_t sample_idx)
{
    /* Generate random crop position using rejection sampling */
    uint32_t max_x = src_width - crop_width;
    uint32_t max_y = src_height - crop_height;
    
    uint32_t op_id = (sample_idx << 16) | 0x0001;  /* Crop X */
    uint32_t crop_x = ct_prng_uniform(seed, epoch, op_id, max_x + 1);
    
    op_id = (sample_idx << 16) | 0x0002;  /* Crop Y */
    uint32_t crop_y = ct_prng_uniform(seed, epoch, op_id, max_y + 1);
    
    /* Copy cropped region */
    for (uint32_t y = 0; y < crop_height; y++) {
        for (uint32_t x = 0; x < crop_width; x++) {
            uint32_t src_idx = (crop_y + y) * src_width + (crop_x + x);
            uint32_t dst_idx = y * crop_width + x;
            output->data[dst_idx] = input->data[src_idx];
        }
    }
    
    /* Update dimensions */
    output->dims[0] = crop_height;
    output->dims[1] = crop_width;
    output->total_elements = crop_width * crop_height;
}

/*===========================================================================*/
/* ct_augment_gaussian_noise (CT-MATH-001 §6.3)                              */
/*===========================================================================*/

static void gaussian_noise(ct_sample_t *sample,
                           int32_t noise_std,
                           uint64_t seed,
                           uint32_t epoch,
                           uint32_t sample_idx,
                           ct_fault_flags_t *faults)
{
    /* Add deterministic Gaussian noise using Box-Muller transform */
    for (uint32_t i = 0; i < sample->total_elements; i += 2) {
        /* Generate two uniform random numbers */
        uint32_t op_id_1 = (sample_idx << 16) | (0x1000 + i);
        uint32_t op_id_2 = (sample_idx << 16) | (0x1000 + i + 1);
        
        uint64_t u1 = ct_prng(seed, epoch, op_id_1);
        uint64_t u2 = ct_prng(seed, epoch, op_id_2);
        
        /* Map to [0, 1) in Q16.16 */
        int32_t u1_fixed = (int32_t)((u1 >> 32) & 0xFFFF0000);
        int32_t u2_fixed = (int32_t)((u2 >> 32) & 0xFFFF0000);
        
        /* Simplified noise: n = std * (u - 0.5) * 2 */
        /* This gives uniform noise in [-std, +std] as approximation */
        int32_t noise_1 = dvm_mul_q16(noise_std, dvm_sub32(u1_fixed, FIXED_HALF, faults), faults);
        noise_1 = dvm_add32(noise_1, noise_1, faults);  /* × 2 */
        
        int32_t noise_2 = dvm_mul_q16(noise_std, dvm_sub32(u2_fixed, FIXED_HALF, faults), faults);
        noise_2 = dvm_add32(noise_2, noise_2, faults);  /* × 2 */
        
        /* Add noise to samples */
        sample->data[i] = dvm_add32(sample->data[i], noise_1, faults);
        if (i + 1 < sample->total_elements) {
            sample->data[i + 1] = dvm_add32(sample->data[i + 1], noise_2, faults);
        }
    }
}

/*===========================================================================*/
/* ct_augment_sample (CT-MATH-001 §6)                                        */
/*===========================================================================*/

void ct_augment_sample(const ct_augment_ctx_t *ctx,
                       const ct_sample_t *input,
                       ct_sample_t *output,
                       uint32_t sample_idx,
                       ct_fault_flags_t *faults)
{
    /* Copy input to output first */
    memcpy(output, input, sizeof(ct_sample_t));
    
    /* Assume 2D image for augmentations */
    uint32_t height = input->dims[0];
    uint32_t width = (input->ndims > 1) ? input->dims[1] : 1;
    
    /* Apply horizontal flip? */
    if (ctx->flags.h_flip) {
        uint32_t op_id = (sample_idx << 16) | 0x0100;  /* Flip decision */
        uint64_t rand = ct_prng(ctx->seed, ctx->epoch, op_id);
        if ((rand & 0x1) == 1) {  /* 50% probability */
            horizontal_flip(output, width, height);
        }
    }
    
    /* Apply random crop? */
    if (ctx->flags.random_crop && ctx->crop_height > 0 && ctx->crop_width > 0) {
        ct_sample_t temp;
        memcpy(&temp, output, sizeof(ct_sample_t));
        random_crop(&temp, output, width, height,
                    ctx->crop_width, ctx->crop_height,
                    ctx->seed, ctx->epoch, sample_idx);
    }
    
    /* Apply Gaussian noise? */
    if (ctx->flags.gaussian_noise && ctx->noise_std > 0) {
        gaussian_noise(output, ctx->noise_std, ctx->seed, ctx->epoch, sample_idx, faults);
    }
}

/*===========================================================================*/
/* ct_augment_batch                                                           */
/*===========================================================================*/

void ct_augment_batch(const ct_augment_ctx_t *ctx,
                      const ct_batch_t *input,
                      ct_batch_t *output,
                      ct_fault_flags_t *faults)
{
    for (uint32_t i = 0; i < input->batch_size; i++) {
        uint32_t global_idx = input->batch_index * input->batch_size + i;
        ct_augment_sample(ctx, &input->samples[i], &output->samples[i], global_idx, faults);
    }
    
    output->batch_size = input->batch_size;
    output->batch_index = input->batch_index;
    memcpy(output->batch_hash, input->batch_hash, 32);
}
