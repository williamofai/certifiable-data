/**
 * @file prng.c
 * @project Certifiable Data Pipeline
 * @brief Deterministic pseudo-random number generator.
 *
 * @details Pure function PRNG based on SplitMix64. Same seed → same sequence.
 *
 * @traceability CT-MATH-001 §5, SRS-003-AUGMENT, SRS-004-SHUFFLE
 * @compliance MISRA-C:2012, ISO 26262, IEC 62304
 *
 * @author William Murray
 * @copyright Copyright (c) 2026 The Murray Family Innovation Trust. All rights reserved.
 * @license Licensed under the GPL-3.0 (Open Source) or Commercial License.
 *          For commercial licensing: william@fstopify.com
 */

#include "prng.h"
#include <stdint.h>

/*===========================================================================*/
/* SplitMix64 hash function                                                   */
/*===========================================================================*/

static uint64_t splitmix64(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

/*===========================================================================*/
/* ct_prng (CT-MATH-001 §5)                                                  */
/*===========================================================================*/

uint64_t ct_prng(uint64_t seed, uint32_t epoch, uint32_t op_id)
{
    /* Construct unique state from seed, epoch, op_id */
    uint64_t state = seed;
    state ^= ((uint64_t)epoch) << 32;
    state ^= (uint64_t)op_id;
    
    /* Two rounds of SplitMix64 for avalanche */
    state = splitmix64(state);
    state = splitmix64(state);
    
    return state;
}

/*===========================================================================*/
/* ct_prng_uniform (CT-MATH-001 §5.3)                                        */
/*===========================================================================*/

uint32_t ct_prng_uniform(uint64_t seed, uint32_t epoch, uint32_t op_id, uint32_t n)
{
    if (n == 0) {
        return 0;
    }
    
    if (n == 1) {
        return 0;
    }
    
    uint64_t rand = ct_prng(seed, epoch, op_id);
    
    /* For small n, use rejection sampling to avoid modulo bias */
    if (n <= 65536) {
        uint32_t threshold = (0xFFFFFFFFU / n) * n;
        uint32_t val = (uint32_t)(rand & 0xFFFFFFFFU);
        
        /* Bounded loop - maximum 4 iterations statistically */
        for (int i = 0; i < 4 && val >= threshold; i++) {
            rand = splitmix64(rand);
            val = (uint32_t)(rand & 0xFFFFFFFFU);
        }
        
        return val % n;
    }
    
    /* For large n, direct modulo is acceptable */
    return (uint32_t)(rand % n);
}
