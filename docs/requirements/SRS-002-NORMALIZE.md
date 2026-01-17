# SRS-002-NORMALIZE: Normalisation Requirements Specification

**Project:** Certifiable Data  
**Module:** Normalize  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the normalisation module, which transforms raw data values to zero-mean, unit-variance representation using precomputed statistics.

**Traceability:**
- CT-MATH-001 v2.0.0 §4 (Normalisation)
- CT-STRUCT-001 v2.0.1 §6

---

## 2. Scope

The normalisation module provides:
1. Per-element normalisation using precomputed mean and inverse standard deviation
2. Per-channel normalisation for multi-channel data
3. Batch normalisation (applying to all samples in a batch)

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Mean (μ) | Precomputed channel mean in Q16.16 |
| Inverse std (σ⁻¹) | Precomputed 1/σ in Q16.16 |
| Normalised value | x̂ = (x − μ) × σ⁻¹ |
| Channel | Independent data dimension (e.g., RGB channels) |

---

## 4. Requirements

### 4.1 Core Normalisation

#### REQ-NORM-001: Normalisation Formula
The module SHALL compute normalised values as:
```
x̂ = (x − μ) × σ⁻¹
```
Where all operations use DVM primitives.

**Verification:** Unit test with known inputs and expected outputs.

#### REQ-NORM-002: Subtraction Operation
The module SHALL compute (x − μ) using DVM_Sub32 with saturation.

**Verification:** Unit test including overflow cases.

#### REQ-NORM-003: Multiplication Operation
The module SHALL compute the product using DVM_Mul64 (widening) followed by DVM_RoundShiftR_RNE(result, 16).

**Verification:** Unit test verifying no intermediate overflow.

#### REQ-NORM-004: Rounding
The module SHALL use Round-to-Nearest-Even (RNE) for the final result.

**Verification:** Test vectors from CT-MATH-001 §3.6.

#### REQ-NORM-005: Saturation
If the result exceeds FIXED_MAX or is below FIXED_MIN, the module SHALL saturate and set the appropriate fault flag.

**Verification:** Unit test with extreme values.

---

### 4.2 Precomputed Statistics

#### REQ-NORM-010: Offline Computation
Statistics (μ, σ⁻¹) SHALL be computed offline and provided at initialisation.

**Verification:** API review — no runtime statistics computation functions.

#### REQ-NORM-011: No Runtime Statistics
The module SHALL NOT compute mean or variance at runtime.

**Verification:** Code review, static analysis.

#### REQ-NORM-012: Inverse Standard Deviation
The module SHALL use precomputed σ⁻¹ to avoid runtime division.

**Verification:** Code review.

#### REQ-NORM-013: Statistics Validation
The module SHALL validate that num_channels ≤ CT_MAX_CHANNELS.

**Verification:** Unit test with invalid channel count.

---

### 4.3 Per-Channel Normalisation

#### REQ-NORM-020: Channel Independence
Each channel SHALL be normalised independently with its own (μ, σ⁻¹) pair.

**Verification:** Unit test with multi-channel data.

#### REQ-NORM-021: Channel Order
Channels SHALL be processed in order: channel 0 first, channel C−1 last.

**Verification:** Unit test verifying deterministic order.

#### REQ-NORM-022: Channel Indexing
For a sample with shape [C, H, W], element (c, y, x) SHALL use statistics for channel c.

**Verification:** Unit test with 3-channel image.

---

### 4.4 Sample Normalisation

#### REQ-NORM-030: Sample Processing
The module SHALL normalise all elements of a sample in row-major order.

**Verification:** Unit test verifying element order.

#### REQ-NORM-031: Shape Preservation
The output sample SHALL have the same shape as the input sample.

**Verification:** Unit test verifying shape.

#### REQ-NORM-032: In-Place Operation
The module SHALL support in-place normalisation (input buffer = output buffer).

**Verification:** Unit test with in-place operation.

---

### 4.5 Determinism

#### REQ-NORM-040: Bit-Identical Results
Normalisation SHALL produce bit-identical results across platforms for the same inputs.

**Verification:** Cross-platform test.

#### REQ-NORM-041: No Floating Point
The module SHALL NOT use floating-point arithmetic.

**Verification:** Static analysis.

---

### 4.6 Fault Handling

#### REQ-NORM-050: Overflow Fault
The module SHALL set FAULT_OVERFLOW when saturation to FIXED_MAX occurs.

**Verification:** Unit test.

#### REQ-NORM-051: Underflow Fault
The module SHALL set FAULT_UNDERFLOW when saturation to FIXED_MIN occurs.

**Verification:** Unit test.

#### REQ-NORM-052: Fault Propagation
A fault during any element normalisation SHALL set the flag but continue processing remaining elements.

**Verification:** Unit test verifying all elements processed.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Normalise a single value.
 * @traceability REQ-NORM-001 through REQ-NORM-005
 */
fixed_t ct_normalize(fixed_t x, fixed_t mean, fixed_t inv_std,
                     ct_fault_flags_t *faults);

/**
 * @brief Normalise a sample (all channels).
 * @traceability REQ-NORM-020 through REQ-NORM-032
 */
void ct_normalize_sample(const ct_sample_t *in, ct_sample_t *out,
                         const ct_norm_config_t *config,
                         ct_fault_flags_t *faults);

/**
 * @brief Validate normalisation configuration.
 * @traceability REQ-NORM-013
 */
bool ct_norm_config_valid(const ct_norm_config_t *config);
```

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-NORM-001 | REQ-NORM-001 | x=65536 (1.0), μ=0, σ⁻¹=65536 → 65536 |
| T-NORM-002 | REQ-NORM-001 | x=131072 (2.0), μ=65536, σ⁻¹=65536 → 65536 |
| T-NORM-003 | REQ-NORM-004 | Verify RNE on tie cases |
| T-NORM-004 | REQ-NORM-005 | Large x causing overflow → FIXED_MAX + fault |
| T-NORM-005 | REQ-NORM-020 | 3-channel sample with different stats per channel |
| T-NORM-006 | REQ-NORM-032 | In-place normalisation |
| T-NORM-007 | REQ-NORM-040 | Cross-platform identity |

---

## 7. Test Vectors

| x (Q16.16) | μ (Q16.16) | σ⁻¹ (Q16.16) | Expected x̂ | Notes |
|------------|------------|--------------|-------------|-------|
| 65536 | 0 | 65536 | 65536 | (1−0)×1 = 1 |
| 131072 | 65536 | 65536 | 65536 | (2−1)×1 = 1 |
| 0 | 65536 | 65536 | -65536 | (0−1)×1 = −1 |
| 65536 | 0 | 32768 | 32768 | (1−0)×0.5 = 0.5 |
| 65536 | 0 | 131072 | 131072 | (1−0)×2 = 2 |
| 196608 | 65536 | 65536 | 131072 | (3−1)×1 = 2 |

---

## 8. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-NORM-001 | §4.1 | — | T-NORM-001, T-NORM-002 |
| REQ-NORM-002 | §4.2 | — | T-NORM-001 |
| REQ-NORM-003 | §4.2 | — | T-NORM-001 |
| REQ-NORM-004 | §3.6 | — | T-NORM-003 |
| REQ-NORM-005 | §4.2 | §3 | T-NORM-004 |
| REQ-NORM-020 | §4.4 | §6.2 | T-NORM-005 |
| REQ-NORM-040 | §14.1 | — | T-NORM-007 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
