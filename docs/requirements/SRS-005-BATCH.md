# SRS-005-BATCH: Batch Requirements Specification

**Project:** Certifiable Data  
**Module:** Batch  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the batch module, which constructs batches of samples with deterministic ordering and Merkle commitment.

**Traceability:**
- CT-MATH-001 v2.0.0 §9 (Batching)
- CT-STRUCT-001 v2.0.1 §10

---

## 2. Scope

The batch module provides:
1. Batch construction from shuffled indices
2. Sample reference tracking (original and shuffled indices)
3. Batch Merkle root computation
4. Incomplete final batch handling

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Batch | Group of B samples processed together |
| Batch size (B) | Maximum samples per batch |
| Batch index | Position of batch within epoch [0, K−1] |
| Batches per epoch (K) | ceil(N / B) |
| Merkle root | Root hash of sample hash tree |

---

## 4. Requirements

### 4.1 Batch Construction

#### REQ-BATCH-001: Sample Selection
For batch b in epoch e, sample i in the batch SHALL be:
```
global_idx = b × B + i
shuffled_idx = π_e(global_idx)
sample = dataset[shuffled_idx]
```

**Verification:** Unit test with known permutation.

#### REQ-BATCH-002: Batch Size
Each batch SHALL contain min(B, N − b × B) samples.

**Verification:** Unit test with N not divisible by B.

#### REQ-BATCH-003: Reference Tracking
Each sample reference SHALL record both original_index (b × B + i) and shuffled_index (π_e(original)).

**Verification:** Unit test verifying both indices.

#### REQ-BATCH-004: Index Bounds
The module SHALL verify global_idx < N before accessing dataset.

**Verification:** Unit test with boundary conditions.

---

### 4.2 Batches Per Epoch

#### REQ-BATCH-010: Batch Count
The number of batches per epoch SHALL be:
```
K = (N + B − 1) / B    // Ceiling division
```

**Verification:** Unit test with various N and B.

#### REQ-BATCH-011: Final Batch
The final batch (index K−1) MAY contain fewer than B samples.

**Verification:** Unit test.

#### REQ-BATCH-012: No Padding
Incomplete batches SHALL NOT be padded with dummy samples.

**Verification:** Code review.

---

### 4.3 Batch Metadata

#### REQ-BATCH-020: Epoch Field
Each batch SHALL record its epoch number.

**Verification:** Unit test.

#### REQ-BATCH-021: Batch Index Field
Each batch SHALL record its batch index within the epoch.

**Verification:** Unit test.

#### REQ-BATCH-022: Batch Size Field
Each batch SHALL record its actual size (may be < B for final batch).

**Verification:** Unit test.

---

### 4.4 Merkle Commitment

#### REQ-BATCH-030: Sample Hashing
Each sample in the batch SHALL be hashed using ct_hash_sample.

**Verification:** Unit test.

#### REQ-BATCH-031: Merkle Root
The batch Merkle root SHALL be computed from sample hashes using ct_merkle_root.

**Verification:** Unit test.

#### REQ-BATCH-032: Batch Hash
The batch hash SHALL be:
```
H_batch = SHA256(0x02 || merkle_root || epoch || batch_index || batch_size)
```
Where epoch, batch_index, batch_size are uint32_le.

**Verification:** Unit test with known values.

#### REQ-BATCH-033: Hash Storage
The batch structure SHALL store both merkle_root and batch_hash.

**Verification:** Structure review.

---

### 4.5 Pipeline Integration

#### REQ-BATCH-040: Augmentation Before Hashing
Samples SHALL be augmented BEFORE hashing (if augmentation is enabled).

**Verification:** Integration test.

#### REQ-BATCH-041: Normalisation Before Hashing
Samples SHALL be normalised BEFORE hashing (if normalisation is enabled).

**Verification:** Integration test.

#### REQ-BATCH-042: Hash Represents Final Data
The Merkle root SHALL represent the data as it will be consumed by training.

**Verification:** Integration test verifying hash matches training input.

---

### 4.6 Determinism

#### REQ-BATCH-050: Bit-Identical Results
Batch construction SHALL produce bit-identical results across platforms.

**Verification:** Cross-platform test.

#### REQ-BATCH-051: Hash Determinism
The same (dataset, config, seed, epoch, batch_index) SHALL always produce the same batch_hash.

**Verification:** Repeated calls.

---

### 4.7 Fault Handling

#### REQ-BATCH-060: Index Out of Bounds
If any index exceeds N−1, the module SHALL set FAULT_DOMAIN.

**Verification:** Unit test.

#### REQ-BATCH-061: Fault Invalidates Hash
If any fault occurs during batch construction, the batch_hash SHALL be invalidated (set to zero).

**Verification:** Unit test.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Compute number of batches per epoch.
 * @traceability REQ-BATCH-010
 */
uint32_t ct_batches_per_epoch(uint32_t N, uint32_t B);

/**
 * @brief Compute size of specific batch.
 * @traceability REQ-BATCH-002
 */
uint32_t ct_batch_size(uint32_t N, uint32_t B, uint32_t batch_idx);

/**
 * @brief Construct a batch.
 * @traceability REQ-BATCH-001 through REQ-BATCH-033
 */
void ct_batch_construct(ct_batch_t *batch, const ct_dataset_t *dataset,
                        const ct_permute_params_t *permute,
                        uint32_t batch_size, uint32_t batch_idx,
                        ct_fault_flags_t *faults);

/**
 * @brief Construct batch with augmentation and normalisation.
 * @traceability REQ-BATCH-040 through REQ-BATCH-042
 */
void ct_batch_construct_full(ct_batch_t *batch, const ct_dataset_t *dataset,
                             const ct_permute_params_t *permute,
                             const ct_augment_config_t *aug_config,
                             const ct_norm_config_t *norm_config,
                             uint64_t seed, const ct_hash_t dataset_hash,
                             uint32_t batch_size, uint32_t batch_idx,
                             fixed_t *work_buf, uint32_t work_buf_size,
                             ct_fault_flags_t *faults);
```

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-BATCH-001 | REQ-BATCH-001 | Sample selection with known permutation |
| T-BATCH-002 | REQ-BATCH-002 | Batch size for N=100, B=32 (final batch = 4) |
| T-BATCH-003 | REQ-BATCH-003 | Reference tracking |
| T-BATCH-004 | REQ-BATCH-010 | Batches per epoch: N=100, B=32 → K=4 |
| T-BATCH-005 | REQ-BATCH-011 | Final batch size |
| T-BATCH-006 | REQ-BATCH-030 | Sample hashing |
| T-BATCH-007 | REQ-BATCH-031 | Merkle root with 4 samples |
| T-BATCH-008 | REQ-BATCH-032 | Batch hash computation |
| T-BATCH-009 | REQ-BATCH-050 | Cross-platform identity |
| T-BATCH-010 | REQ-BATCH-061 | Fault invalidates hash |

---

## 7. Test Vectors

### Batches Per Epoch

| N | B | Expected K |
|---|---|------------|
| 100 | 32 | 4 |
| 100 | 100 | 1 |
| 100 | 101 | 1 |
| 60000 | 64 | 938 |
| 1 | 32 | 1 |

### Batch Sizes

| N | B | batch_idx | Expected size |
|---|---|-----------|---------------|
| 100 | 32 | 0 | 32 |
| 100 | 32 | 1 | 32 |
| 100 | 32 | 2 | 32 |
| 100 | 32 | 3 | 4 |
| 60000 | 64 | 937 | 32 |

---

## 8. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-BATCH-001 | §9.1 | §10.1 | T-BATCH-001 |
| REQ-BATCH-002 | §9.1 | §10.2 | T-BATCH-002 |
| REQ-BATCH-010 | §9.2 | — | T-BATCH-004 |
| REQ-BATCH-030 | §10.2 | §4 | T-BATCH-006 |
| REQ-BATCH-031 | §10.4 | §4 | T-BATCH-007 |
| REQ-BATCH-032 | §10.5 | §10.2 | T-BATCH-008 |
| REQ-BATCH-050 | §14.1 | — | T-BATCH-009 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
