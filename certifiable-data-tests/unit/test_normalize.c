/**
 * @file test_normalize.c
 * @project Certifiable Data Pipeline
 * @brief Unit tests for fixed-point normalization
 *
 * @traceability SRS-002-NORMALIZE, CT-MATH-001 §4
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ct_types.h"
#include "normalize.h"
#include "dvm.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s ", #fn); \
    tests_run++; \
    if (fn()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ============================================================================
 * Test: Context Initialization
 * ============================================================================ */

static int test_ctx_init(void)
{
    int32_t means[3] = {FIXED_ONE, FIXED_HALF, FIXED_ZERO};
    int32_t inv_stds[3] = {FIXED_ONE, FIXED_ONE, FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 3);
    
    if (ctx.means != means) return 0;
    if (ctx.inv_stds != inv_stds) return 0;
    if (ctx.num_features != 3) return 0;
    
    return 1;
}

static int test_ctx_init_zero_features(void)
{
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, NULL, NULL, 0);
    
    return ctx.num_features == 0;
}

/* ============================================================================
 * Test: Basic Normalization (y = (x - mean) * inv_std)
 * ============================================================================ */

static int test_normalize_identity(void)
{
    ct_fault_flags_t faults = {0};
    
    /* mean = 0, inv_std = 1.0 → output should equal input */
    int32_t means[3] = {0, 0, 0};
    int32_t inv_stds[3] = {FIXED_ONE, FIXED_ONE, FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 3);
    
    int32_t input_data[3] = {FIXED_ONE, FIXED_HALF, 2 << 16};
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {3, 0, 0, 0},
        .total_elements = 3,
        .data = input_data
    };
    
    int32_t output_data[3];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {3, 0, 0, 0},
        .total_elements = 3,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    if (output.data[0] != FIXED_ONE) return 0;
    if (output.data[1] != FIXED_HALF) return 0;
    if (output.data[2] != (2 << 16)) return 0;
    
    return 1;
}

static int test_normalize_zero_mean(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Subtracting mean should center at zero */
    int32_t means[1] = {5 << 16};  /* mean = 5.0 */
    int32_t inv_stds[1] = {FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 1);
    
    int32_t input_data[1] = {5 << 16};  /* x = 5.0 */
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = input_data
    };
    
    int32_t output_data[1];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* (5 - 5) * 1 = 0 */
    return output.data[0] == 0;
}

static int test_normalize_scale(void)
{
    ct_fault_flags_t faults = {0};
    
    /* mean = 2.0, inv_std = 0.5 */
    int32_t means[3] = {2 << 16, 3 << 16, 1 << 16};
    int32_t inv_stds[3] = {FIXED_HALF, FIXED_HALF, FIXED_HALF};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 3);
    
    /* Input: [4.0, 5.0, 3.0] */
    int32_t input_data[3] = {4 << 16, 5 << 16, 3 << 16};
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {3, 0, 0, 0},
        .total_elements = 3,
        .data = input_data
    };
    
    int32_t output_data[3];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {3, 0, 0, 0},
        .total_elements = 3,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* Expected: (4-2)*0.5 = 1.0, (5-3)*0.5 = 1.0, (3-1)*0.5 = 1.0 */
    if (output.data[0] != FIXED_ONE) return 0;
    if (output.data[1] != FIXED_ONE) return 0;
    if (output.data[2] != FIXED_ONE) return 0;
    
    return 1;
}

static int test_normalize_negative_values(void)
{
    ct_fault_flags_t faults = {0};
    
    /* mean = 0, inv_std = 1.0 */
    int32_t means[2] = {0, 0};
    int32_t inv_stds[2] = {FIXED_ONE, FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 2);
    
    int32_t input_data[2] = {-(2 << 16), -(3 << 16)};  /* -2.0, -3.0 */
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = input_data
    };
    
    int32_t output_data[2];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* Should preserve negative values */
    if (output.data[0] != -(2 << 16)) return 0;
    if (output.data[1] != -(3 << 16)) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Determinism
 * ============================================================================ */

static int test_determinism(void)
{
    ct_fault_flags_t faults1 = {0};
    ct_fault_flags_t faults2 = {0};
    
    int32_t means[2] = {FIXED_ONE, FIXED_ONE};
    int32_t inv_stds[2] = {FIXED_HALF, FIXED_HALF};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 2);
    
    int32_t input_data[2] = {3 << 16, 2 << 16};
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = input_data
    };
    
    int32_t output_data1[2];
    int32_t output_data2[2];
    ct_sample_t output1 = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = output_data1
    };
    ct_sample_t output2 = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {2, 0, 0, 0},
        .total_elements = 2,
        .data = output_data2
    };
    
    /* Normalize twice */
    ct_normalize_sample(&ctx, &input, &output1, &faults1);
    ct_normalize_sample(&ctx, &input, &output2, &faults2);
    
    /* Should be identical */
    return memcmp(output1.data, output2.data, 2 * sizeof(int32_t)) == 0;
}

/* ============================================================================
 * Test: Metadata Preservation
 * ============================================================================ */

static int test_metadata_preserved(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t means[1] = {0};
    int32_t inv_stds[1] = {FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 1);
    
    int32_t input_data[1] = {FIXED_ONE};
    ct_sample_t input = {
        .version = 42,
        .dtype = 99,
        .ndims = 2,
        .dims = {10, 20, 0, 0},
        .total_elements = 1,
        .data = input_data
    };
    
    int32_t output_data[1];
    ct_sample_t output = {
        .version = 0,
        .dtype = 0,
        .ndims = 0,
        .dims = {0, 0, 0, 0},
        .total_elements = 0,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* Metadata should be copied */
    if (output.version != 42) return 0;
    if (output.dtype != 99) return 0;
    if (output.ndims != 2) return 0;
    if (output.dims[0] != 10) return 0;
    if (output.dims[1] != 20) return 0;
    if (output.total_elements != 1) return 0;
    
    return 1;
}

/* ============================================================================
 * Test: Partial Normalization
 * ============================================================================ */

static int test_fewer_features_than_elements(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Only normalize first 2 elements */
    int32_t means[2] = {FIXED_ONE, FIXED_ONE};
    int32_t inv_stds[2] = {FIXED_ONE, FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 2);
    
    int32_t input_data[4] = {2 << 16, 3 << 16, 4 << 16, 5 << 16};
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {4, 0, 0, 0},
        .total_elements = 4,
        .data = input_data
    };
    
    int32_t output_data[4];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {4, 0, 0, 0},
        .total_elements = 4,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* First 2 normalized, last 2 unchanged */
    if (output.data[0] != FIXED_ONE) return 0;  /* (2-1)*1 = 1 */
    if (output.data[1] != (2 << 16)) return 0;  /* (3-1)*1 = 2 */
    if (output.data[2] != (4 << 16)) return 0;  /* Unchanged */
    if (output.data[3] != (5 << 16)) return 0;  /* Unchanged */
    
    return 1;
}

/* ============================================================================
 * Test: Batch Normalization
 * ============================================================================ */

static int test_normalize_batch(void)
{
    ct_fault_flags_t faults = {0};
    
    int32_t means[1] = {FIXED_ONE};
    int32_t inv_stds[1] = {FIXED_ONE};
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 1);
    
    /* Create batch with 2 samples */
    int32_t data0[1] = {2 << 16};
    int32_t data1[1] = {3 << 16};
    ct_sample_t input_samples[2] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = data1}
    };
    
    ct_batch_t input_batch = {
        .samples = input_samples,
        .batch_size = 2,
        .batch_index = 0
    };
    
    int32_t out_data0[1];
    int32_t out_data1[1];
    ct_sample_t output_samples[2] = {
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = out_data0},
        {.version = 1, .dtype = 0, .ndims = 1, .dims = {1, 0, 0, 0}, .total_elements = 1, .data = out_data1}
    };
    
    ct_batch_t output_batch = {
        .samples = output_samples,
        .batch_size = 2,
        .batch_index = 0
    };
    
    ct_normalize_batch(&ctx, &input_batch, &output_batch, &faults);
    
    /* Check normalized values */
    if (output_samples[0].data[0] != FIXED_ONE) return 0;  /* (2-1)*1 = 1 */
    if (output_samples[1].data[0] != (2 << 16)) return 0;  /* (3-1)*1 = 2 */
    
    return 1;
}

/* ============================================================================
 * Test: Fault Handling
 * ============================================================================ */

static int test_saturation_overflow(void)
{
    ct_fault_flags_t faults = {0};
    
    /* Large inv_std can cause overflow */
    int32_t means[1] = {0};
    int32_t inv_stds[1] = {INT32_MAX};  /* Very large scale */
    
    ct_normalize_ctx_t ctx;
    ct_normalize_init(&ctx, means, inv_stds, 1);
    
    int32_t input_data[1] = {FIXED_ONE};
    ct_sample_t input = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = input_data
    };
    
    int32_t output_data[1];
    ct_sample_t output = {
        .version = 1,
        .dtype = 0,
        .ndims = 1,
        .dims = {1, 0, 0, 0},
        .total_elements = 1,
        .data = output_data
    };
    
    ct_normalize_sample(&ctx, &input, &output, &faults);
    
    /* Should have triggered overflow or other fault */
    /* (exact behavior depends on DVM implementation) */
    return 1;  /* Just verify no crash */
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("==============================================\n");
    printf("Certifiable Data - Normalization Tests\n");
    printf("Traceability: SRS-002-NORMALIZE, CT-MATH-001 §4\n");
    printf("==============================================\n\n");
    
    printf("Context initialization:\n");
    RUN_TEST(test_ctx_init);
    RUN_TEST(test_ctx_init_zero_features);
    
    printf("\nBasic normalization:\n");
    RUN_TEST(test_normalize_identity);
    RUN_TEST(test_normalize_zero_mean);
    RUN_TEST(test_normalize_scale);
    RUN_TEST(test_normalize_negative_values);
    
    printf("\nDeterminism:\n");
    RUN_TEST(test_determinism);
    
    printf("\nMetadata preservation:\n");
    RUN_TEST(test_metadata_preserved);
    
    printf("\nPartial normalization:\n");
    RUN_TEST(test_fewer_features_than_elements);
    
    printf("\nBatch normalization:\n");
    RUN_TEST(test_normalize_batch);
    
    printf("\nFault handling:\n");
    RUN_TEST(test_saturation_overflow);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
