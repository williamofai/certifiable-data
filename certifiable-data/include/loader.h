/**
 * @file loader.h
 * @project Certifiable Data Pipeline
 * @brief Data loading interface.
 *
 * @traceability SRS-001-LOADER, CT-STRUCT-001 §11
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_LOADER_H
#define CT_LOADER_H

#include "ct_types.h"

/**
 * @brief Load dataset from CSV file.
 * @param filepath Path to CSV file
 * @param dataset Dataset structure (pre-allocated)
 * @return Number of samples loaded, or -1 on error
 * @traceability REQ-LOAD-001, REQ-LOAD-002
 */
int ct_load_csv(const char *filepath, ct_dataset_t *dataset);

/**
 * @brief Load dataset from binary file.
 * @param filepath Path to binary file
 * @param dataset Dataset structure (pre-allocated)
 * @return Number of samples loaded, or -1 on error
 * @traceability REQ-LOAD-003, REQ-LOAD-004
 */
int ct_load_binary(const char *filepath, ct_dataset_t *dataset);

/**
 * @brief Initialize dataset structure.
 * @param dataset Dataset to initialize
 * @param samples Pre-allocated sample array
 * @param num_samples Number of samples
 * @traceability REQ-LOAD-005
 */
void ct_dataset_init(ct_dataset_t *dataset,
                     ct_sample_t *samples,
                     uint32_t num_samples);

#endif /* CT_LOADER_H */
