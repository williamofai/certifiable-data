/**
 * @file test_batch.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for batch construction
 *
 * @traceability SRS-005-BATCH, CT-MATH-001 §9
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ct_types.h"
#include "batch.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Batch Initialization
 * ============================================================================ */

static int test_batch_init(void)
{
    ct_sample_t samples[10];
    ct_hash_t sample_hashes[10];
    
    ct_batch_t batch;
    ct_batch_init(&batch, samples, sample_hashes, 10);
    
    if (batch.samples != samples) return 0;
    if (batch.sample_hashes != sample_hashes) return 0;
    if (batch.batch_size != 10) return 0;
    if (batch.batch_index != 0) return 0;
    
    /* Hash should be zeroed */
    for (int i = 0; i < 32; i++) {
        if (batch.batch_hash[i] != 0) return 0;
    }
    
    return 1;
}

static int test_batch_init_size_zero(void)
{
    ct_batch_t batch;
    ct_batch_init(&batch, NULL, NULL, 0);
    
    return batch.batch_size == 0;
}

/* ============================================================================
 * Test: Batch Fill
 * ============================================================================ */

static int test_batch_fill_basic(void)
{
    /* Create dataset with 3 samples */
    int32_t data0[2] = {FIXED_ONE, FIXED_ZERO};
    int32_t data1[2] = {FIXED_HALF, FIXED_ONE};
    int32_t data2[2] = {2 << 16, 3 << 16};
    
    ct_sample_t dataset_samples[3] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {2, 0, 0, 0}, .total_elements = 2, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {2, 0, 0, 0}, .total_elements = 2, .data = data1},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {2, 0, 0, 0}, .total_elements = 2, .data = data2}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 3,
        .dataset_hash = {0}
    };
    
    /* Create batch */
    ct_sample_t batch_samples[2];
    ct_hash_t batch_hashes[2];
    ct_batch_t batch;
    ct_batch_init(&batch, batch_samples, batch_hashes, 2);
    
    /* Fill batch */
    ct_batch_fill(&batch, &dataset, 0, 0, 0x123456789ABCDEF0ULL);
    
    /* Verify batch index set */
    if (batch.batch_index != 0) return 0;
    
    /* Batch hash should be non-zero after fill */
    int hash_nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (batch.batch_hash[i] != 0) {
            hash_nonzero = 1;
            break;
        }
    }
    
    return hash_nonzero;
}

static int test_batch_fill_deterministic(void)
{
    /* Create dataset */
    int32_t data0[1] = {FIXED_ONE};
    int32_t data1[1] = {FIXED_HALF};
    ct_sample_t dataset_samples[2] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data1}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 2,
        .dataset_hash = {0}
    };
    
    /* Create two batches */
    ct_sample_t batch1_samples[2];
    ct_hash_t batch1_hashes[2];
    ct_batch_t batch1;
    ct_batch_init(&batch1, batch1_samples, batch1_hashes, 2);
    
    ct_sample_t batch2_samples[2];
    ct_hash_t batch2_hashes[2];
    ct_batch_t batch2;
    ct_batch_init(&batch2, batch2_samples, batch2_hashes, 2);
    
    /* Fill both with same parameters */
    uint64_t seed = 0xFEDCBA9876543210ULL;
    uint32_t epoch = 0;
    uint32_t batch_index = 0;
    
    ct_batch_fill(&batch1, &dataset, batch_index, epoch, seed);
    ct_batch_fill(&batch2, &dataset, batch_index, epoch, seed);
    
    /* Hashes should be identical */
    return memcmp(batch1.batch_hash, batch2.batch_hash, 32) == 0;
}

static int test_batch_fill_different_epoch(void)
{
    /* Create dataset with MULTIPLE samples so shuffle matters */
    int32_t data0[1] = {FIXED_ONE};
    int32_t data1[1] = {FIXED_HALF};
    int32_t data2[1] = {2 << 16};
    ct_sample_t dataset_samples[3] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data1},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data2}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 3,
        .dataset_hash = {0}
    };
    
    /* Create two batches */
    ct_sample_t batch1_samples[2];
    ct_hash_t batch1_hashes[2];
    ct_batch_t batch1;
    ct_batch_init(&batch1, batch1_samples, batch1_hashes, 2);
    
    ct_sample_t batch2_samples[2];
    ct_hash_t batch2_hashes[2];
    ct_batch_t batch2;
    ct_batch_init(&batch2, batch2_samples, batch2_hashes, 2);
    
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    /* Fill with different epochs */
    ct_batch_fill(&batch1, &dataset, 0, 0, seed);
    ct_batch_fill(&batch2, &dataset, 0, 1, seed);
    
    /* Hashes should differ (different shuffle) */
    return memcmp(batch1.batch_hash, batch2.batch_hash, 32) != 0;
}

static int test_batch_fill_partial_last_batch(void)
{
    /* Dataset with 5 samples, batch size 3 */
    int32_t data[5][1];
    ct_sample_t dataset_samples[5];
    for (int i = 0; i < 5; i++) {
        data[i][0] = i << 16;
        dataset_samples[i].version = 1;
        dataset_samples[i].dtype = 0;
        dataset_samples[i].ndims = 1;
        dataset_samples[i].dims[0] = 1;
        dataset_samples[i].total_elements = 1;
        dataset_samples[i].data = data[i];
    }
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 5,
        .dataset_hash = {0}
    };
    
    /* Last batch (index 1) should have only 2 samples */
    ct_sample_t batch_samples[3];
    ct_hash_t batch_hashes[3];
    ct_batch_t batch;
    ct_batch_init(&batch, batch_samples, batch_hashes, 3);
    
    ct_batch_fill(&batch, &dataset, 1, 0, 0x123456789ABCDEF0ULL);
    
    /* Should handle partial batch without crashing */
    return 1;
}

/* ============================================================================
 * Test: Batch Get Sample
 * ============================================================================ */

static int test_get_sample_valid_index(void)
{
    int32_t data0[1] = {FIXED_ONE};
    int32_t data1[1] = {FIXED_HALF};
    ct_sample_t samples[2] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data1}
    };
    
    ct_hash_t hashes[2];
    ct_batch_t batch;
    ct_batch_init(&batch, samples, hashes, 2);
    
    const ct_sample_t *s0 = ct_batch_get_sample(&batch, 0);
    const ct_sample_t *s1 = ct_batch_get_sample(&batch, 1);
    
    if (s0 != &samples[0]) return 0;
    if (s1 != &samples[1]) return 0;
    
    return 1;
}

static int test_get_sample_invalid_index(void)
{
    ct_sample_t samples[2];
    ct_hash_t hashes[2];
    ct_batch_t batch;
    ct_batch_init(&batch, samples, hashes, 2);
    
    /* Out of bounds should return NULL */
    const ct_sample_t *s = ct_batch_get_sample(&batch, 10);
    
    return s == NULL;
}

/* ============================================================================
 * Test: Batch Verification
 * ============================================================================ */

static int test_batch_verify_valid(void)
{
    /* Create dataset */
    int32_t data0[1] = {FIXED_ONE};
    ct_sample_t dataset_samples[1] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 1,
        .dataset_hash = {0}
    };
    
    /* Create batch */
    ct_sample_t batch_samples[1];
    ct_hash_t batch_hashes[1];
    ct_batch_t batch;
    ct_batch_init(&batch, batch_samples, batch_hashes, 1);
    
    /* Fill batch */
    ct_batch_fill(&batch, &dataset, 0, 0, 0x123456789ABCDEF0ULL);
    
    /* Verify should pass */
    int valid = ct_batch_verify(&batch);
    
    return valid == 1;
}

static int test_batch_verify_corrupted(void)
{
    /* Create dataset */
    int32_t data0[1] = {FIXED_ONE};
    ct_sample_t dataset_samples[1] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 1,
        .dataset_hash = {0}
    };
    
    /* Create batch */
    ct_sample_t batch_samples[1];
    ct_hash_t batch_hashes[1];
    ct_batch_t batch;
    ct_batch_init(&batch, batch_samples, batch_hashes, 1);
    
    /* Fill batch */
    ct_batch_fill(&batch, &dataset, 0, 0, 0x123456789ABCDEF0ULL);
    
    /* Corrupt batch hash */
    batch.batch_hash[0] ^= 0xFF;
    
    /* Verify should fail */
    int valid = ct_batch_verify(&batch);
    
    return valid == 0;
}

static int test_batch_verify_deterministic(void)
{
    /* Create dataset */
    int32_t data0[1] = {FIXED_ONE};
    ct_sample_t dataset_samples[1] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0}
    };
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 1,
        .dataset_hash = {0}
    };
    
    /* Create batch */
    ct_sample_t batch_samples[1];
    ct_hash_t batch_hashes[1];
    ct_batch_t batch;
    ct_batch_init(&batch, batch_samples, batch_hashes, 1);
    
    /* Fill batch */
    ct_batch_fill(&batch, &dataset, 0, 0, 0x123456789ABCDEF0ULL);
    
    /* Verify multiple times should always pass */
    int valid1 = ct_batch_verify(&batch);
    int valid2 = ct_batch_verify(&batch);
    int valid3 = ct_batch_verify(&batch);
    
    return (valid1 == 1) && (valid2 == 1) && (valid3 == 1);
}

/* ============================================================================
 * Test: Multiple Batches from Same Dataset
 * ============================================================================ */

static int test_multiple_batches_different_hashes(void)
{
    /* Create dataset with 6 samples */
    int32_t data[6][1];
    ct_sample_t dataset_samples[6];
    for (int i = 0; i < 6; i++) {
        data[i][0] = i << 16;
        dataset_samples[i].version = 1;
        dataset_samples[i].dtype = 0;
        dataset_samples[i].ndims = 1;
        dataset_samples[i].dims[0] = 1;
        dataset_samples[i].total_elements = 1;
        dataset_samples[i].data = data[i];
    }
    
    ct_dataset_t dataset = {
        .samples = dataset_samples,
        .num_samples = 6,
        .dataset_hash = {0}
    };
    
    /* Create two batches */
    ct_sample_t batch0_samples[3];
    ct_hash_t batch0_hashes[3];
    ct_batch_t batch0;
    ct_batch_init(&batch0, batch0_samples, batch0_hashes, 3);
    
    ct_sample_t batch1_samples[3];
    ct_hash_t batch1_hashes[3];
    ct_batch_t batch1;
    ct_batch_init(&batch1, batch1_samples, batch1_hashes, 3);
    
    uint64_t seed = 0x123456789ABCDEF0ULL;
    uint32_t epoch = 0;
    
    /* Fill different batch indices */
    ct_batch_fill(&batch0, &dataset, 0, epoch, seed);
    ct_batch_fill(&batch1, &dataset, 1, epoch, seed);
    
    /* Different batches should have different hashes */
    return memcmp(batch0.batch_hash, batch1.batch_hash, 32) != 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - Batch Construction Tests\n");
    printf("Traceability: SRS-005-BATCH, CT-MATH-001 §9\n");
    printf("==============================================\n\n");
    
    printf("Batch initialization:\n");
    RUN_TEST(test_batch_init);
    RUN_TEST(test_batch_init_size_zero);
    
    printf("\nBatch fill:\n");
    RUN_TEST(test_batch_fill_basic);
    RUN_TEST(test_batch_fill_deterministic);
    RUN_TEST(test_batch_fill_different_epoch);
    RUN_TEST(test_batch_fill_partial_last_batch);
    
    printf("\nGet sample:\n");
    RUN_TEST(test_get_sample_valid_index);
    RUN_TEST(test_get_sample_invalid_index);
    
    printf("\nBatch verification:\n");
    RUN_TEST(test_batch_verify_valid);
    RUN_TEST(test_batch_verify_corrupted);
    RUN_TEST(test_batch_verify_deterministic);
    
    printf("\nMultiple batches:\n");
    RUN_TEST(test_multiple_batches_different_hashes);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
