/**
 * @file sha256.h
 * @project Certifiable Data Pipeline
 * @brief SHA-256 cryptographic hash interface.
 *
 * @traceability SRS-006-MERKLE
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#ifndef CT_SHA256_H
#define CT_SHA256_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief SHA-256 context structure.
 */
typedef struct {
    uint8_t data[64];      /**< Current block */
    uint32_t datalen;      /**< Number of bytes in current block */
    uint64_t bitlen;       /**< Total message length in bits */
    uint32_t state[8];     /**< Hash state */
} ct_sha256_ctx_t;

/**
 * @brief Initialize SHA-256 context.
 * @param ctx Context to initialize
 */
void ct_sha256_init(ct_sha256_ctx_t *ctx);

/**
 * @brief Update SHA-256 with data.
 * @param ctx Context
 * @param data Data to hash
 * @param len Length of data in bytes
 */
void ct_sha256_update(ct_sha256_ctx_t *ctx, const uint8_t data[], size_t len);

/**
 * @brief Finalize SHA-256 and produce hash.
 * @param ctx Context
 * @param hash Output hash (32 bytes)
 */
void ct_sha256_final(ct_sha256_ctx_t *ctx, uint8_t hash[32]);

#endif /* CT_SHA256_H */
