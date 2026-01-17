# SRS-004-SHUFFLE: Shuffle Requirements Specification

**Project:** Certifiable Data  
**Module:** Shuffle  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the shuffle module, which provides deterministic, bijective index permutation using a cycle-walking Feistel network.

**Traceability:**
- CT-MATH-001 v2.0.0 §7 (Feistel Permutation)
- CT-STRUCT-001 v2.0.1 §9

---

## 2. Scope

The shuffle module provides:
1. Bijective permutation π: [0, N−1] → [0, N−1]
2. Epoch-based shuffling (different permutation per epoch)
3. Deterministic index mapping

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Permutation | Bijective mapping from input indices to output indices |
| Feistel network | Symmetric cipher construction providing bijectivity |
| Cycle-walking | Technique to restrict permutation to arbitrary range |
| Epoch | Training iteration over full dataset |

---

## 4. Requirements

### 4.1 Permutation Properties

#### REQ-SHUF-001: Bijectivity
For any (N, seed, epoch), the permutation SHALL be bijective:
```
|{π(i) : i ∈ [0, N)}| = N
```

**Verification:** Exhaustive test for N ∈ {1, 2, 10, 100, 1000, 60000}.

#### REQ-SHUF-002: Determinism
The same (N, seed, epoch, index) SHALL always produce the same output.

**Verification:** Repeated calls with same inputs.

#### REQ-SHUF-003: Epoch Variation
Different epochs SHALL produce different permutations:
```
π_{epoch=0}(i) ≠ π_{epoch=1}(i)  (with high probability)
```

**Verification:** Compare permutations across epochs.

#### REQ-SHUF-004: Seed Sensitivity
Different seeds SHALL produce different permutations.

**Verification:** Compare permutations across seeds.

---

### 4.2 Feistel Network

#### REQ-SHUF-010: Balanced Split
The Feistel network SHALL use balanced halves:
```
half_bits = (k + 1) / 2
```
Where k = ceil_log2(N).

**Verification:** Unit test for odd and even k values.

#### REQ-SHUF-011: Four Rounds
The Feistel network SHALL use exactly 4 rounds.

**Verification:** Code review.

#### REQ-SHUF-012: Round Function
The round function SHALL be:
```
F(R, seed, epoch, round) = uint32_le(SHA256(seed || epoch || R || round)[0:4])
```

**Verification:** Test vectors.

#### REQ-SHUF-013: Feistel Structure
Each round SHALL update (L, R) as:
```
(L, R) ← (R, L XOR F(R))
```

**Verification:** Unit test.

---

### 4.3 Cycle-Walking

#### REQ-SHUF-020: Range Restriction
If the Feistel output i ≥ N, the module SHALL re-apply the Feistel network until i < N.

**Verification:** Unit test with N that is not a power of 2.

#### REQ-SHUF-021: Bounded Iterations
Cycle-walking SHALL have a bounded loop guard (max = 2^k iterations).

**Verification:** Code review.

#### REQ-SHUF-022: Fallback on Exhaustion
If the loop guard is exceeded, the module SHALL set FAULT_DOMAIN and return index MOD N.

**Verification:** Theoretical analysis (probability is negligible for reasonable N).

---

### 4.4 Parameter Initialization

#### REQ-SHUF-030: ceil_log2 Computation
The module SHALL compute k = ceil_log2(N) as specified in CT-MATH-001 §7.4.

**Verification:** Unit test with N = 1, 2, 3, 4, 100, 1024, 60000.

#### REQ-SHUF-031: Parameter Precomputation
The module SHALL precompute half_bits, half_mask, and range at initialization.

**Verification:** Unit test verifying precomputed values.

#### REQ-SHUF-032: Edge Case N=0
If N = 0, the module SHALL set FAULT_DOMAIN and return 0.

**Verification:** Unit test.

#### REQ-SHUF-033: Edge Case N=1
If N = 1, the module SHALL return 0 (only valid index).

**Verification:** Unit test.

---

### 4.5 Test Vectors

#### REQ-SHUF-040: Reference Test Vectors
The module SHALL pass the following test vectors from CT-MATH-001 §7.5:

| N | seed | epoch | index | expected |
|---|------|-------|-------|----------|
| 100 | 0x123456789ABCDEF0 | 0 | 0 | 26 |
| 100 | 0x123456789ABCDEF0 | 0 | 99 | 41 |
| 100 | 0x123456789ABCDEF0 | 1 | 0 | 66 |
| 60000 | 0xFEDCBA9876543210 | 0 | 0 | 26382 |
| 60000 | 0xFEDCBA9876543210 | 0 | 59999 | 20774 |

**Verification:** Unit test with exact values.

---

### 4.6 Determinism

#### REQ-SHUF-050: Bit-Identical Results
Permutation SHALL produce bit-identical results across platforms.

**Verification:** Cross-platform test.

#### REQ-SHUF-051: No Floating Point
The module SHALL NOT use floating-point arithmetic.

**Verification:** Static analysis.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Compute ceil_log2(n).
 * @traceability REQ-SHUF-030
 */
uint32_t ct_ceil_log2(uint32_t n);

/**
 * @brief Initialize permutation parameters.
 * @traceability REQ-SHUF-031
 */
void ct_permute_init(ct_permute_params_t *p, uint64_t seed,
                     uint32_t epoch, uint32_t N);

/**
 * @brief Compute permuted index.
 * @traceability REQ-SHUF-001 through REQ-SHUF-022
 */
uint32_t ct_permute_index(const ct_permute_params_t *p, uint32_t index,
                          ct_fault_flags_t *faults);

/**
 * @brief Feistel round function.
 * @traceability REQ-SHUF-012
 */
uint32_t ct_feistel_round(uint32_t R, uint64_t seed, uint32_t epoch,
                          uint8_t round);
```

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-SHUF-001 | REQ-SHUF-001 | Bijectivity for N=100 |
| T-SHUF-002 | REQ-SHUF-001 | Bijectivity for N=60000 |
| T-SHUF-003 | REQ-SHUF-002 | Determinism (repeated calls) |
| T-SHUF-004 | REQ-SHUF-003 | Epoch variation |
| T-SHUF-005 | REQ-SHUF-010 | Balanced split for k=7 (odd) |
| T-SHUF-006 | REQ-SHUF-010 | Balanced split for k=8 (even) |
| T-SHUF-007 | REQ-SHUF-030 | ceil_log2 test vectors |
| T-SHUF-008 | REQ-SHUF-033 | Edge case N=1 |
| T-SHUF-009 | REQ-SHUF-040 | Reference test vectors |
| T-SHUF-010 | REQ-SHUF-050 | Cross-platform identity |

---

## 7. ceil_log2 Test Vectors

| n | expected k |
|---|------------|
| 0 | 0 |
| 1 | 0 |
| 2 | 1 |
| 3 | 2 |
| 4 | 2 |
| 5 | 3 |
| 100 | 7 |
| 128 | 7 |
| 129 | 8 |
| 1024 | 10 |
| 60000 | 16 |

---

## 8. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-SHUF-001 | §7.1 | — | T-SHUF-001, T-SHUF-002 |
| REQ-SHUF-010 | §7.2 | §9.1 | T-SHUF-005, T-SHUF-006 |
| REQ-SHUF-012 | §7.3 | — | T-SHUF-009 |
| REQ-SHUF-020 | §7.2 | — | T-SHUF-001 |
| REQ-SHUF-030 | §7.4 | — | T-SHUF-007 |
| REQ-SHUF-040 | §7.5 | — | T-SHUF-009 |
| REQ-SHUF-050 | §14.1 | — | T-SHUF-010 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
