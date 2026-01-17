/**
 * @file ct_types.h
 * @project Certifiable Data Pipeline
 * @brief Core type definitions and constants.
 *
 * @traceability CT-STRUCT-001, CT-MATH-001 §2
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_TYPES_H
#define CT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Fixed-Point Constants (Q16.16)                                            */
/*===========================================================================*/

#define FIXED_SHIFT   16
#define FIXED_ONE     (1 << FIXED_SHIFT)       /* 65536 = 0x00010000 */
#define FIXED_HALF    (1 << (FIXED_SHIFT - 1)) /* 32768 = 0x00008000 */
#define FIXED_ZERO    0
#define FIXED_MAX     INT32_MAX                /* 0x7FFFFFFF */
#define FIXED_MIN     INT32_MIN                /* 0x80000000 */
#define FIXED_EPS     1                        /* 0x00000001 */

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#define CT_MAX_DIMS           4
#define CT_MAX_SAMPLE_SIZE    (1024 * 1024)  /* 1M elements max */

/*===========================================================================*/
/* Domain Separation Prefixes                                                */
/*===========================================================================*/

#define CT_DOMAIN_LEAF        0x00
#define CT_DOMAIN_INTERNAL    0x01
#define CT_DOMAIN_BATCH       0x02
#define CT_DOMAIN_PROVENANCE  0x03
#define CT_DOMAIN_EPOCH_CHAIN 0x04

/*===========================================================================*/
/* Hash Type                                                                  */
/*===========================================================================*/

typedef uint8_t ct_hash_t[32];

/*===========================================================================*/
/* Fault Flags (CT-STRUCT-001 §3)                                            */
/*===========================================================================*/

typedef struct {
    uint32_t overflow       : 1;  /**< Saturated high */
    uint32_t underflow      : 1;  /**< Saturated low */
    uint32_t div_zero       : 1;  /**< Division by zero */
    uint32_t domain         : 1;  /**< Invalid input */
    uint32_t precision      : 1;  /**< Precision loss detected */
    uint32_t grad_floor     : 1;  /**< Excessive zero gradients */
    uint32_t chain_invalid  : 1;  /**< Merkle chain invalid */
    uint32_t _reserved      : 25;
} ct_fault_flags_t;

/*===========================================================================*/
/* Sample (CT-STRUCT-001 §5)                                                 */
/*===========================================================================*/

typedef struct {
    uint32_t version;              /**< Format version (1) */
    uint32_t dtype;                /**< Data type (0=Q16.16) */
    uint32_t ndims;                /**< Number of dimensions */
    uint32_t dims[CT_MAX_DIMS];    /**< Dimension sizes */
    uint32_t total_elements;       /**< Product of dims */
    int32_t *data;                 /**< Sample data (Q16.16) */
} ct_sample_t;

/*===========================================================================*/
/* Normalization Context (CT-STRUCT-001 §6)                                  */
/*===========================================================================*/

typedef struct {
    const int32_t *means;          /**< Mean values (Q16.16) */
    const int32_t *inv_stds;       /**< Inverse standard deviations (Q16.16) */
    uint32_t num_features;         /**< Number of features */
} ct_normalize_ctx_t;

/*===========================================================================*/
/* Augmentation (CT-STRUCT-001 §8)                                           */
/*===========================================================================*/

typedef struct {
    uint32_t h_flip        : 1;    /**< Enable horizontal flip */
    uint32_t v_flip        : 1;    /**< Enable vertical flip */
    uint32_t random_crop   : 1;    /**< Enable random crop */
    uint32_t gaussian_noise: 1;    /**< Enable Gaussian noise */
    uint32_t _reserved     : 28;
} ct_augment_flags_t;

typedef struct {
    uint64_t seed;                 /**< Random seed */
    uint32_t epoch;                /**< Current epoch */
    ct_augment_flags_t flags;      /**< Enabled augmentations */
    uint32_t crop_width;           /**< Crop width (if random_crop) */
    uint32_t crop_height;          /**< Crop height (if random_crop) */
    int32_t noise_std;             /**< Noise std dev (Q16.16) */
} ct_augment_ctx_t;

/*===========================================================================*/
/* Shuffle Context (CT-STRUCT-001 §9)                                        */
/*===========================================================================*/

typedef struct {
    uint64_t seed;                 /**< Random seed */
    uint32_t epoch;                /**< Current epoch */
} ct_shuffle_ctx_t;

/*===========================================================================*/
/* Batch (CT-STRUCT-001 §10)                                                 */
/*===========================================================================*/

typedef struct {
    ct_sample_t *samples;          /**< Array of samples */
    ct_hash_t *sample_hashes;      /**< Hash of each sample */
    uint32_t batch_size;           /**< Maximum samples in batch */
    uint32_t batch_index;          /**< Index of this batch */
    ct_hash_t batch_hash;          /**< Merkle root of samples */
} ct_batch_t;

/*===========================================================================*/
/* Dataset (CT-STRUCT-001 §11)                                               */
/*===========================================================================*/

typedef struct {
    ct_sample_t *samples;          /**< Array of samples */
    uint32_t num_samples;          /**< Number of samples */
    ct_hash_t dataset_hash;        /**< Hash of entire dataset */
} ct_dataset_t;

/*===========================================================================*/
/* Provenance Chain (CT-STRUCT-001 §12)                                      */
/*===========================================================================*/

typedef struct {
    uint32_t current_epoch;        /**< Current epoch number */
    uint32_t total_epochs;         /**< Total epochs completed */
    ct_hash_t current_hash;        /**< h_e */
    ct_hash_t prev_hash;           /**< h_{e-1} */
} ct_provenance_t;

#endif /* CT_TYPES_H */
