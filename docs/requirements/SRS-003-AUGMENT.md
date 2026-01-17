# SRS-003-AUGMENT: Augmentation Requirements Specification

**Project:** Certifiable Data  
**Module:** Augment  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the data augmentation module, which applies deterministic transforms to samples using PRNG-driven decisions.

**Traceability:**
- CT-MATH-001 v2.0.0 §5 (PRNG), §6 (Rejection Sampling), §8 (Augmentation)
- CT-STRUCT-001 v2.0.1 §7, §8

---

## 2. Scope

The augmentation module provides:
1. Horizontal flip
2. Vertical flip
3. Random crop
4. Brightness adjustment
5. Additive noise
6. Fixed-order augmentation pipeline

---

## 3. Definitions

| Term | Definition |
|------|------------|
| PRNG | Pure function: PRNG(seed, op_id, step) → uint32 |
| op_id | Operation identifier encoding context |
| Augmentation pipeline | Fixed sequence: crop → hflip → vflip → brightness → noise |

---

## 4. Requirements

### 4.1 PRNG Core

#### REQ-AUG-001: Pure PRNG Function
The PRNG SHALL be a pure function with signature:
```
PRNG(seed: uint64, op_id: uint64, step: uint64) → uint32
```

**Verification:** Unit test verifying same inputs produce same outputs.

#### REQ-AUG-002: PRNG Algorithm
The PRNG SHALL use the SplitMix64-derived algorithm specified in CT-MATH-001 §5.3.

**Verification:** Test vectors.

#### REQ-AUG-003: op_id Construction
The op_id SHALL be constructed as specified in CT-MATH-001 §5.2:
```
op_id = hash64(dataset_hash[0:8] || epoch || sample_idx || augment_id || element_idx_lo)
```

**Verification:** Unit test verifying op_id computation.

#### REQ-AUG-004: Augmentation IDs
The module SHALL use the following augmentation IDs:
| ID | Operation |
|----|-----------|
| 0x01 | Horizontal flip |
| 0x02 | Vertical flip |
| 0x03 | Crop Y offset |
| 0x04 | Crop X offset |
| 0x05 | Brightness |
| 0x06 | Noise |

**Verification:** Code review.

---

### 4.2 Rejection Sampling

#### REQ-AUG-010: Unbiased Random
For random offsets, the module SHALL use rejection sampling as specified in CT-MATH-001 §6.2.

**Verification:** Statistical test for uniformity.

#### REQ-AUG-011: Bounded Loop
Rejection sampling SHALL have a bounded loop guard (max 128 iterations).

**Verification:** Unit test.

#### REQ-AUG-012: Fallback on Exhaustion
If the loop guard is exceeded, the module SHALL set FAULT_DOMAIN and return a biased result.

**Verification:** Unit test forcing loop exhaustion.

---

### 4.3 Horizontal Flip

#### REQ-AUG-020: Flip Decision
The flip decision SHALL be: `(PRNG(seed, op_id, 0) & 1) == 1`

**Verification:** Unit test with known seed.

#### REQ-AUG-021: Flip Transform
When enabled and decision is true, element [y, x] SHALL be copied from [y, W−1−x].

**Verification:** Unit test verifying pixel positions.

#### REQ-AUG-022: Flip Disabled
When disabled, the sample SHALL be copied without modification, but PRNG output SHALL still be computed.

**Verification:** Unit test verifying PRNG consumption.

---

### 4.4 Vertical Flip

#### REQ-AUG-030: VFlip Decision
The flip decision SHALL be: `(PRNG(seed, op_id, 0) & 1) == 1`

**Verification:** Unit test with known seed.

#### REQ-AUG-031: VFlip Transform
When enabled and decision is true, element [y, x] SHALL be copied from [H−1−y, x].

**Verification:** Unit test verifying pixel positions.

#### REQ-AUG-032: VFlip Disabled
When disabled, the sample SHALL be copied without modification, but PRNG output SHALL still be computed.

**Verification:** Unit test.

---

### 4.5 Random Crop

#### REQ-AUG-040: Crop Offset Computation
Offsets SHALL be computed using unbiased_random:
```
offset_y = unbiased_random(seed, op_id_y, max_y + 1)
offset_x = unbiased_random(seed, op_id_x, max_x + 1)
```

**Verification:** Unit test with known seed.

#### REQ-AUG-041: Crop Transform
Element [y, x] in output SHALL be copied from [offset_y + y, offset_x + x] in input.

**Verification:** Unit test.

#### REQ-AUG-042: Crop Disabled
When disabled, the module SHALL return a center crop:
```
offset_y = max_y / 2
offset_x = max_x / 2
```
PRNG outputs SHALL still be computed.

**Verification:** Unit test verifying center crop and PRNG consumption.

#### REQ-AUG-043: Crop Size Validation
The module SHALL verify crop_height ≤ input_height and crop_width ≤ input_width.

**Verification:** Unit test with invalid crop size.

---

### 4.6 Brightness Adjustment

#### REQ-AUG-050: Brightness Factor Computation
The brightness factor SHALL be computed as specified in CT-MATH-001 §8.5:
```
r = PRNG(seed, op_id, 0)
r_signed = (r & 0xFFFF) - 32768
offset = DVM_RoundShiftR_RNE(r_signed × delta, 15)
factor = FIXED_ONE + offset
```

**Verification:** Unit test with known seed and delta.

#### REQ-AUG-051: Brightness Application
Each element SHALL be transformed as:
```
out[i] = DVM_RoundShiftR_RNE(in[i] × factor, 16)
```

**Verification:** Unit test.

#### REQ-AUG-052: Brightness Disabled
When disabled, the sample SHALL be copied without modification, but PRNG output SHALL still be computed.

**Verification:** Unit test.

---

### 4.7 Additive Noise

#### REQ-AUG-060: Noise Generation
For each element i, noise SHALL be generated as:
```
r = PRNG(seed, op_id, i)
r_signed = (r & 0xFFFF) - 32768
noise = DVM_RoundShiftR_RNE(r_signed × amplitude, 15)
```

**Verification:** Unit test.

#### REQ-AUG-061: Noise Application
Each element SHALL be transformed as:
```
out[i] = DVM_Add32(in[i], noise)
```

**Verification:** Unit test.

#### REQ-AUG-062: Noise Disabled
When disabled, the sample SHALL be copied without modification, but PRNG outputs SHALL still be computed for all elements.

**Verification:** Unit test verifying PRNG consumption matches enabled case.

---

### 4.8 Augmentation Pipeline

#### REQ-AUG-070: Pipeline Order
Augmentations SHALL be applied in fixed order:
1. Random crop
2. Horizontal flip
3. Vertical flip
4. Brightness adjustment
5. Additive noise

**Verification:** Unit test verifying order.

#### REQ-AUG-071: Pipeline Determinism
The same (seed, dataset_hash, epoch, sample_idx, config) SHALL always produce the same output.

**Verification:** Determinism test.

#### REQ-AUG-072: PRNG Consumption Independence
PRNG consumption SHALL be independent of which augmentations are enabled.

**Verification:** Unit test comparing PRNG state after full pipeline vs selective pipeline.

---

### 4.9 Determinism

#### REQ-AUG-080: Bit-Identical Results
Augmentation SHALL produce bit-identical results across platforms.

**Verification:** Cross-platform test.

#### REQ-AUG-081: No Floating Point
The module SHALL NOT use floating-point arithmetic.

**Verification:** Static analysis.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Pure PRNG function.
 * @traceability REQ-AUG-001, REQ-AUG-002
 */
uint32_t ct_prng(uint64_t seed, uint64_t op_id, uint64_t step);

/**
 * @brief Construct op_id for augmentation.
 * @traceability REQ-AUG-003
 */
uint64_t ct_make_op_id(const ct_hash_t dataset_hash, uint32_t epoch,
                       uint32_t sample_idx, uint8_t augment_id,
                       uint8_t element_idx_lo);

/**
 * @brief Unbiased random in range [0, range).
 * @traceability REQ-AUG-010 through REQ-AUG-012
 */
uint32_t ct_unbiased_random(uint64_t seed, uint64_t op_id, uint32_t range,
                            ct_fault_flags_t *faults);

/**
 * @brief Apply full augmentation pipeline.
 * @traceability REQ-AUG-070 through REQ-AUG-072
 */
void ct_augment_pipeline(const ct_sample_t *in, ct_sample_t *out,
                         const ct_augment_config_t *config,
                         uint64_t seed, const ct_hash_t dataset_hash,
                         uint32_t epoch, uint32_t sample_idx,
                         ct_fault_flags_t *faults);
```

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-AUG-001 | REQ-AUG-001 | PRNG determinism |
| T-AUG-002 | REQ-AUG-002 | PRNG test vectors |
| T-AUG-003 | REQ-AUG-020 | HFlip with known seed |
| T-AUG-004 | REQ-AUG-030 | VFlip with known seed |
| T-AUG-005 | REQ-AUG-040 | Crop offsets with known seed |
| T-AUG-006 | REQ-AUG-050 | Brightness factor computation |
| T-AUG-007 | REQ-AUG-060 | Noise generation |
| T-AUG-008 | REQ-AUG-070 | Pipeline order verification |
| T-AUG-009 | REQ-AUG-072 | PRNG consumption independence |
| T-AUG-010 | REQ-AUG-080 | Cross-platform identity |

---

## 7. PRNG Test Vectors

| seed | op_id | step | Expected |
|------|-------|------|----------|
| 0x0 | 0x0 | 0 | TBD (compute at impl) |
| 0x123456789ABCDEF0 | 0x0 | 0 | TBD |
| 0x123456789ABCDEF0 | 0x1 | 0 | TBD |
| 0x123456789ABCDEF0 | 0x0 | 1 | TBD |

---

## 8. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-AUG-001 | §5.1 | §7.2 | T-AUG-001 |
| REQ-AUG-002 | §5.3 | — | T-AUG-002 |
| REQ-AUG-010 | §6.2 | — | T-AUG-005 |
| REQ-AUG-020 | §8.2 | §8.1 | T-AUG-003 |
| REQ-AUG-070 | §8.1 | §8.1 | T-AUG-008 |
| REQ-AUG-080 | §14.1 | — | T-AUG-010 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
