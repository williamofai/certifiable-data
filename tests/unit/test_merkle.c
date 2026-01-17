/**
 * @file test_merkle.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for Merkle trees and provenance
 *
 * @traceability SRS-006-MERKLE, CT-MATH-001 §10
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ct_types.h"
#include "merkle.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Sample Hashing
 * ============================================================================ */

static int test_hash_sample_deterministic(void)
{
    /* Create sample */
    int32_t data[4] = {FIXED_ONE, FIXED_HALF, FIXED_ZERO, -FIXED_ONE};
    ct_sample_t sample = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {4, 0, 0, 0},
        .total_elements = 4,
        .data = data
    };
    
    /* Hash twice */
    ct_hash_t hash1, hash2;
    ct_hash_sample(&sample, hash1);
    ct_hash_sample(&sample, hash2);
    
    /* Must be identical */
    return memcmp(hash1, hash2, 32) == 0;
}

static int test_hash_sample_different_data(void)
{
    int32_t data1[2] = {FIXED_ONE, FIXED_ZERO};
    int32_t data2[2] = {FIXED_ONE, FIXED_ONE};
    
    ct_sample_t sample1 = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = data1
    };
    
    ct_sample_t sample2 = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = data2
    };
    
    ct_hash_t hash1, hash2;
    ct_hash_sample(&sample1, hash1);
    ct_hash_sample(&sample2, hash2);
    
    /* Different data should give different hash */
    return memcmp(hash1, hash2, 32) != 0;
}

static int test_hash_sample_sensitive_to_metadata(void)
{
    int32_t data[1] = {FIXED_ONE};
    
    ct_sample_t sample1 = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = data
    };
    
    ct_sample_t sample2 = {
        .version = 2,  /* Different version */
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = data
    };
    
    ct_hash_t hash1, hash2;
    ct_hash_sample(&sample1, hash1);
    ct_hash_sample(&sample2, hash2);
    
    /* Different metadata should give different hash */
    return memcmp(hash1, hash2, 32) != 0;
}

static int test_hash_sample_nonzero(void)
{
    int32_t data[1] = {FIXED_ONE};
    ct_sample_t sample = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = data
    };
    
    ct_hash_t hash;
    ct_hash_sample(&sample, hash);
    
    /* Hash should not be all zeros */
    int nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (hash[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    
    return nonzero;
}

/* ============================================================================
 * Test: Internal Node Hashing
 * ============================================================================ */

static int test_hash_internal_deterministic(void)
{
    ct_hash_t left = {0x01, 0x02, 0x03, 0x04};
    ct_hash_t right = {0x05, 0x06, 0x07, 0x08};
    
    ct_hash_t hash1, hash2;
    ct_hash_internal(left, right, hash1);
    ct_hash_internal(left, right, hash2);
    
    return memcmp(hash1, hash2, 32) == 0;
}

static int test_hash_internal_order_matters(void)
{
    ct_hash_t left = {0x01, 0x02};
    ct_hash_t right = {0x03, 0x04};
    
    ct_hash_t hash1, hash2;
    ct_hash_internal(left, right, hash1);
    ct_hash_internal(right, left, hash2);  /* Swapped */
    
    /* Order should matter */
    return memcmp(hash1, hash2, 32) != 0;
}

/* ============================================================================
 * Test: Merkle Root
 * ============================================================================ */

static int test_merkle_root_single_leaf(void)
{
    ct_hash_t leaf = {0xAB, 0xCD, 0xEF};
    
    ct_hash_t root;
    ct_merkle_root((const ct_hash_t *)&leaf, 1, root);
    
    /* Single leaf root should equal the leaf */
    return memcmp(root, leaf, 32) == 0;
}

static int test_merkle_root_two_leaves(void)
{
    ct_hash_t leaves[2];
    memset(leaves[0], 0x11, 32);
    memset(leaves[1], 0x22, 32);
    
    ct_hash_t root;
    ct_merkle_root((const ct_hash_t *)leaves, 2, root);
    
    /* Root should be non-zero */
    int nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (root[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    
    return nonzero;
}

static int test_merkle_root_deterministic(void)
{
    ct_hash_t leaves[4];
    for (int i = 0; i < 4; i++) {
        memset(leaves[i], i, 32);
    }
    
    ct_hash_t root1, root2;
    ct_merkle_root((const ct_hash_t *)leaves, 4, root1);
    ct_merkle_root((const ct_hash_t *)leaves, 4, root2);
    
    return memcmp(root1, root2, 32) == 0;
}

static int test_merkle_root_zero_leaves(void)
{
    ct_hash_t root;
    ct_merkle_root(NULL, 0, root);
    
    /* Zero leaves should give zero root */
    for (int i = 0; i < 32; i++) {
        if (root[i] != 0) return 0;
    }
    
    return 1;
}

static int test_merkle_root_odd_count(void)
{
    /* Test with odd number of leaves */
    ct_hash_t leaves[3];
    memset(leaves[0], 0x11, 32);
    memset(leaves[1], 0x22, 32);
    memset(leaves[2], 0x33, 32);
    
    ct_hash_t root;
    ct_merkle_root((const ct_hash_t *)leaves, 3, root);
    
    /* Should handle odd count without crashing */
    return 1;
}

/* ============================================================================
 * Test: Batch Hashing
 * ============================================================================ */

static int test_hash_batch(void)
{
    /* Create batch with 2 samples */
    int32_t data0[1] = {FIXED_ONE};
    int32_t data1[1] = {FIXED_HALF};
    ct_sample_t samples[2] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data1}
    };
    
    ct_hash_t sample_hashes[2];
    ct_batch_t batch = {
        .samples = samples,
        .sample_hashes = sample_hashes,
        .batch_size = 2,
        .batch_index = 0
    };
    
    /* Compute sample hashes */
    ct_hash_sample(&samples[0], sample_hashes[0]);
    ct_hash_sample(&samples[1], sample_hashes[1]);
    
    /* Compute batch hash */
    ct_hash_t batch_hash;
    ct_hash_batch(&batch, batch_hash);
    
    /* Should be non-zero */
    int nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (batch_hash[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    
    return nonzero;
}

static int test_hash_batch_deterministic(void)
{
    int32_t data0[1] = {FIXED_ONE};
    ct_sample_t samples[1] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0}
    };
    
    ct_hash_t sample_hashes[1];
    ct_batch_t batch = {
        .samples = samples,
        .sample_hashes = sample_hashes,
        .batch_size = 1,
        .batch_index = 0
    };
    
    ct_hash_sample(&samples[0], sample_hashes[0]);
    
    ct_hash_t hash1, hash2;
    ct_hash_batch(&batch, hash1);
    ct_hash_batch(&batch, hash2);
    
    return memcmp(hash1, hash2, 32) == 0;
}

/* ============================================================================
 * Test: Epoch Hashing
 * ============================================================================ */

static int test_hash_epoch(void)
{
    ct_hash_t batch_hashes[3];
    memset(batch_hashes[0], 0x11, 32);
    memset(batch_hashes[1], 0x22, 32);
    memset(batch_hashes[2], 0x33, 32);
    
    ct_hash_t epoch_hash;
    ct_hash_epoch((const ct_hash_t *)batch_hashes, 3, epoch_hash);
    
    /* Should be non-zero */
    int nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (epoch_hash[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    
    return nonzero;
}

/* ============================================================================
 * Test: Provenance Chain
 * ============================================================================ */

static int test_provenance_init(void)
{
    ct_provenance_t prov;
    ct_hash_t dataset_hash = {0};
    ct_hash_t config_hash = {0};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    ct_provenance_init(&prov, dataset_hash, config_hash, seed);
    
    if (prov.current_epoch != 0) return 0;
    if (prov.total_epochs != 0) return 0;
    
    /* Hash should be non-zero */
    int nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (prov.current_hash[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    
    return nonzero;
}

static int test_provenance_init_deterministic(void)
{
    ct_hash_t dataset_hash = {0x01};
    ct_hash_t config_hash = {0x02};
    uint64_t seed = 0xABCDEF0123456789ULL;
    
    ct_provenance_t prov1, prov2;
    ct_provenance_init(&prov1, dataset_hash, config_hash, seed);
    ct_provenance_init(&prov2, dataset_hash, config_hash, seed);
    
    /* Same inputs should give same hash */
    return memcmp(prov1.current_hash, prov2.current_hash, 32) == 0;
}

static int test_provenance_advance(void)
{
    ct_provenance_t prov;
    ct_hash_t dataset_hash = {0};
    ct_hash_t config_hash = {0};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    ct_provenance_init(&prov, dataset_hash, config_hash, seed);
    
    /* Save initial hash */
    ct_hash_t initial_hash;
    memcpy(initial_hash, prov.current_hash, 32);
    
    /* Advance epoch */
    ct_hash_t epoch_hash = {0xAB};
    ct_provenance_advance(&prov, epoch_hash);
    
    /* Epoch should increment */
    if (prov.current_epoch != 1) return 0;
    if (prov.total_epochs != 1) return 0;
    
    /* Hash should have changed */
    if (memcmp(prov.current_hash, initial_hash, 32) == 0) return 0;
    
    /* prev_hash should equal initial_hash */
    if (memcmp(prov.prev_hash, initial_hash, 32) != 0) return 0;
    
    return 1;
}

static int test_provenance_chain_deterministic(void)
{
    ct_hash_t dataset_hash = {0x01};
    ct_hash_t config_hash = {0x02};
    uint64_t seed = 0xFEDCBA9876543210ULL;
    
    ct_provenance_t prov1, prov2;
    ct_provenance_init(&prov1, dataset_hash, config_hash, seed);
    ct_provenance_init(&prov2, dataset_hash, config_hash, seed);
    
    ct_hash_t epoch_hash = {0xAB};
    
    /* Advance both */
    ct_provenance_advance(&prov1, epoch_hash);
    ct_provenance_advance(&prov2, epoch_hash);
    
    /* Should produce identical results */
    return memcmp(prov1.current_hash, prov2.current_hash, 32) == 0;
}

static int test_provenance_multiple_epochs(void)
{
    ct_provenance_t prov;
    ct_hash_t dataset_hash = {0};
    ct_hash_t config_hash = {0};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    ct_provenance_init(&prov, dataset_hash, config_hash, seed);
    
    /* Advance multiple epochs */
    ct_hash_t epoch_hash1 = {0x01};
    ct_hash_t epoch_hash2 = {0x02};
    ct_hash_t epoch_hash3 = {0x03};
    
    ct_provenance_advance(&prov, epoch_hash1);
    ct_provenance_advance(&prov, epoch_hash2);
    ct_provenance_advance(&prov, epoch_hash3);
    
    if (prov.current_epoch != 3) return 0;
    if (prov.total_epochs != 3) return 0;
    
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - Merkle & Provenance Tests\n");
    printf("Traceability: SRS-006-MERKLE, CT-MATH-001 §10\n");
    printf("==============================================\n\n");
    
    printf("Sample hashing:\n");
    RUN_TEST(test_hash_sample_deterministic);
    RUN_TEST(test_hash_sample_different_data);
    RUN_TEST(test_hash_sample_sensitive_to_metadata);
    RUN_TEST(test_hash_sample_nonzero);
    
    printf("\nInternal node hashing:\n");
    RUN_TEST(test_hash_internal_deterministic);
    RUN_TEST(test_hash_internal_order_matters);
    
    printf("\nMerkle root:\n");
    RUN_TEST(test_merkle_root_single_leaf);
    RUN_TEST(test_merkle_root_two_leaves);
    RUN_TEST(test_merkle_root_deterministic);
    RUN_TEST(test_merkle_root_zero_leaves);
    RUN_TEST(test_merkle_root_odd_count);
    
    printf("\nBatch hashing:\n");
    RUN_TEST(test_hash_batch);
    RUN_TEST(test_hash_batch_deterministic);
    
    printf("\nEpoch hashing:\n");
    RUN_TEST(test_hash_epoch);
    
    printf("\nProvenance chain:\n");
    RUN_TEST(test_provenance_init);
    RUN_TEST(test_provenance_init_deterministic);
    RUN_TEST(test_provenance_advance);
    RUN_TEST(test_provenance_chain_deterministic);
    RUN_TEST(test_provenance_multiple_epochs);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
