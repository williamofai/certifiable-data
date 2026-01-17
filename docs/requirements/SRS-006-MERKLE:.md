# SRS-006-MERKLE: Merkle & Provenance Requirements Specification

**Project:** Certifiable Data  
**Module:** Merkle  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the Merkle tree and provenance chain module, which provides cryptographic commitment and auditability for data pipelines.

**Traceability:**
- CT-MATH-001 v2.0.0 §10 (Merkle Commitment), §11 (Serialization)
- CT-STRUCT-001 v2.0.1 §4, §12

---

## 2. Scope

The Merkle module provides:
1. Sample hashing with domain separation
2. Merkle tree construction
3. Batch and epoch hashing
4. Dataset provenance chain

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Domain separation | Prefix byte distinguishing node types |
| Merkle tree | Binary hash tree with leaves = data hashes |
| Merkle root | Root node of Merkle tree |
| Provenance chain | Linked sequence of epoch hashes |

---

## 4. Requirements

### 4.1 Domain Separation

#### REQ-MERK-001: Domain Prefixes
The module SHALL use the following domain separation prefixes:
| Domain | Prefix |
|--------|--------|
| Leaf node | 0x00 |
| Internal node | 0x01 |
| Batch metadata | 0x02 |
| Epoch metadata | 0x03 |
| Provenance chain | 0x04 |

**Verification:** Code review.

#### REQ-MERK-002: Prefix Inclusion
Every SHA-256 input SHALL begin with the appropriate domain prefix byte.

**Verification:** Unit test verifying hash inputs.

---

### 4.2 Sample Hashing

#### REQ-MERK-010: Sample Hash Formula
Sample hash SHALL be:
```
H_sample = SHA256(0x00 || serialize(sample))
```

**Verification:** Unit test with known sample.

#### REQ-MERK-011: Sample Serialization
Sample serialization SHALL follow CT-MATH-001 §11.2:
```
version || dtype || ndims || dims[] || data[]
```
All multi-byte values in little-endian.

**Verification:** Unit test.

#### REQ-MERK-012: Shape Inclusion
Sample hash SHALL include shape metadata (version, dtype, ndims, dims).

**Verification:** Unit test with differently-shaped samples producing different hashes.

---

### 4.3 Internal Node Hashing

#### REQ-MERK-020: Node Hash Formula
Internal node hash SHALL be:
```
H_node = SHA256(0x01 || left || right)
```
Where left and right are 32-byte child hashes.

**Verification:** Unit test.

#### REQ-MERK-021: Child Order
Left child SHALL precede right child in the hash input.

**Verification:** Unit test verifying order matters.

---

### 4.4 Merkle Tree Construction

#### REQ-MERK-030: Leaf Level
Level 0 (leaves) SHALL contain sample hashes.

**Verification:** Unit test.

#### REQ-MERK-031: Odd Leaf Handling
If the number of leaves is odd, the last leaf SHALL be duplicated.

**Verification:** Unit test with 1, 3, 5 leaves.

#### REQ-MERK-032: Tree Construction
The tree SHALL be built bottom-up, pairing adjacent nodes at each level.

**Verification:** Unit test.

#### REQ-MERK-033: Root Output
The final single node SHALL be the Merkle root.

**Verification:** Unit test.

#### REQ-MERK-034: Empty Tree
For zero leaves, the Merkle root SHALL be SHA256(0x00).

**Verification:** Unit test.

#### REQ-MERK-035: Single Leaf
For one leaf, the Merkle root SHALL be that leaf's hash (no internal nodes).

**Verification:** Unit test.

---

### 4.5 Batch Hash

#### REQ-MERK-040: Batch Hash Formula
Batch hash SHALL be:
```
H_batch = SHA256(0x02 || merkle_root || epoch || batch_index || batch_size)
```
Where epoch, batch_index, batch_size are uint32_le.

**Verification:** Unit test with known values.

#### REQ-MERK-041: Metadata Encoding
All metadata integers SHALL be encoded as little-endian.

**Verification:** Unit test.

---

### 4.6 Epoch Hash

#### REQ-MERK-050: Epoch Hash Formula
Epoch hash SHALL be:
```
H_epoch = SHA256(0x03 || merkle_root(batch_hashes) || epoch || num_batches)
```
Where epoch and num_batches are uint32_le.

**Verification:** Unit test.

#### REQ-MERK-051: Batch Hash Tree
Batch hashes for the epoch SHALL form a Merkle tree.

**Verification:** Unit test.

---

### 4.7 Provenance Chain

#### REQ-MERK-060: Initial Hash
The initial provenance hash SHALL be:
```
h_0 = SHA256(0x04 || dataset_hash || config_hash || seed)
```
Where seed is uint64_le.

**Verification:** Unit test.

#### REQ-MERK-061: Chain Advancement
Each epoch advances the chain:
```
h_e = SHA256(0x04 || h_{e-1} || H_epoch(e) || e)
```
Where e is uint32_le.

**Verification:** Unit test.

#### REQ-MERK-062: Chain Immutability
Once initialized, dataset_hash, config_hash, and seed SHALL NOT change.

**Verification:** API review.

#### REQ-MERK-063: Fault Termination
If any fault occurs during an epoch, the provenance chain SHALL NOT be advanced.

**Verification:** Unit test.

---

### 4.8 Verification

#### REQ-MERK-070: Batch Verification
Given a batch and its claimed hash, the module SHALL verify by recomputing.

**Verification:** Unit test with valid and tampered batches.

#### REQ-MERK-071: Epoch Verification
Given batch hashes and claimed epoch hash, the module SHALL verify by recomputing.

**Verification:** Unit test.

#### REQ-MERK-072: Verification Result
Verification failure SHALL set FAULT_HASH_MISMATCH.

**Verification:** Unit test.

---

### 4.9 Determinism

#### REQ-MERK-080: Bit-Identical Hashes
The same input SHALL produce bit-identical hashes across platforms.

**Verification:** Cross-platform test.

#### REQ-MERK-081: SHA-256 Compliance
The module SHALL use SHA-256 as specified in FIPS 180-4.

**Verification:** Test vectors from FIPS 180-4.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Hash a sample with domain separation.
 * @traceability REQ-MERK-010 through REQ-MERK-012
 */
void ct_hash_sample(const ct_sample_t *sample, ct_hash_t out);

/**
 * @brief Hash internal Merkle node.
 * @traceability REQ-MERK-020, REQ-MERK-021
 */
void ct_hash_node(const ct_hash_t left, const ct_hash_t right, ct_hash_t out);

/**
 * @brief Compute Merkle root from leaves.
 * @traceability REQ-MERK-030 through REQ-MERK-035
 */
void ct_merkle_root(const ct_hash_t *leaves, uint32_t count, ct_hash_t out);

/**
 * @brief Compute batch hash.
 * @traceability REQ-MERK-040, REQ-MERK-041
 */
void ct_hash_batch(uint32_t epoch, uint32_t batch_index, uint32_t batch_size,
                   const ct_hash_t merkle_root, ct_hash_t out);

/**
 * @brief Compute epoch hash.
 * @traceability REQ-MERK-050, REQ-MERK-051
 */
void ct_hash_epoch(const ct_hash_t *batch_hashes, uint32_t num_batches,
                   uint32_t epoch, ct_hash_t out);

/**
 * @brief Initialize provenance chain.
 * @traceability REQ-MERK-060
 */
void ct_provenance_init(ct_provenance_t *prov, const ct_hash_t dataset_hash,
                        const ct_hash_t config_hash, uint64_t seed);

/**
 * @brief Advance provenance chain.
 * @traceability REQ-MERK-061
 */
void ct_provenance_advance(ct_provenance_t *prov, const ct_hash_t epoch_hash);

/**
 * @brief Verify batch hash.
 * @traceability REQ-MERK-070, REQ-MERK-072
 */
bool ct_verify_batch(const ct_batch_t *batch, const ct_hash_t expected,
                     ct_fault_flags_t *faults);

/**
 * @brief Verify epoch hash.
 * @traceability REQ-MERK-071, REQ-MERK-072
 */
bool ct_verify_epoch(const ct_hash_t *batch_hashes, uint32_t num_batches,
                     uint32_t epoch, const ct_hash_t expected,
                     ct_fault_flags_t *faults);
```

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-MERK-001 | REQ-MERK-001 | Domain prefix constants |
| T-MERK-002 | REQ-MERK-010 | Sample hash with known data |
| T-MERK-003 | REQ-MERK-012 | Different shapes → different hashes |
| T-MERK-004 | REQ-MERK-020 | Internal node hash |
| T-MERK-005 | REQ-MERK-031 | Odd leaf duplication |
| T-MERK-006 | REQ-MERK-034 | Empty tree |
| T-MERK-007 | REQ-MERK-035 | Single leaf |
| T-MERK-008 | REQ-MERK-040 | Batch hash |
| T-MERK-009 | REQ-MERK-050 | Epoch hash |
| T-MERK-010 | REQ-MERK-060 | Provenance init |
| T-MERK-011 | REQ-MERK-061 | Provenance advance |
| T-MERK-012 | REQ-MERK-070 | Batch verification pass |
| T-MERK-013 | REQ-MERK-072 | Verification failure → fault |
| T-MERK-014 | REQ-MERK-080 | Cross-platform identity |
| T-MERK-015 | REQ-MERK-081 | SHA-256 FIPS test vectors |

---

## 7. Test Vectors

### SHA-256 FIPS 180-4 Vectors

| Input | Expected SHA-256 (hex) |
|-------|------------------------|
| "abc" | ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad |
| "" | e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 |

### Merkle Tree Vectors

| Leaves (hex, abbreviated) | Expected Root |
|---------------------------|---------------|
| [H("a")] | SHA256(0x00 || "a") |
| [H("a"), H("b")] | SHA256(0x01 || H("a") || H("b")) |
| [H("a"), H("b"), H("c")] | SHA256(0x01 || SHA256(0x01 || H("a") || H("b")) || SHA256(0x01 || H("c") || H("c"))) |

---

## 8. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-MERK-001 | §10.1 | §4 | T-MERK-001 |
| REQ-MERK-010 | §10.2 | — | T-MERK-002 |
| REQ-MERK-020 | §10.3 | — | T-MERK-004 |
| REQ-MERK-030 | §10.4 | — | T-MERK-005 |
| REQ-MERK-040 | §10.5 | §10.2 | T-MERK-008 |
| REQ-MERK-050 | §10.6 | — | T-MERK-009 |
| REQ-MERK-060 | §10.7 | §12 | T-MERK-010 |
| REQ-MERK-080 | §14.1 | — | T-MERK-014 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
