# SRS-001-LOADER: Data Loader Requirements Specification

**Project:** Certifiable Data  
**Module:** Loader  
**Version:** 1.0.0  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 1. Purpose

This document specifies the requirements for the data loader module, which reads datasets from files and converts them to Q16.16 fixed-point representation.

**Traceability:**
- CT-MATH-001 v2.0.0 §9 (Data Loading), §12 (Decimal Conversion)
- CT-STRUCT-001 v2.0.1 §5, §11, §14

---

## 2. Scope

The loader module provides:
1. Binary tensor file loading (.tensor)
2. CSV file loading with decimal→fixed conversion
3. Statistics file loading (.stat)
4. Dataset hash computation

---

## 3. Definitions

| Term | Definition |
|------|------------|
| Dataset | Collection of N samples with uniform shape |
| Sample | Single data item (e.g., image, feature vector) |
| Q16.16 | Signed 32-bit fixed-point with 16 fractional bits |
| Tensor file | Binary format defined in CT-MATH-001 §9.2 |
| Statistics file | Binary format for normalisation parameters |

---

## 4. Requirements

### 4.1 Tensor File Loading

#### REQ-LOAD-001: Magic Number Validation
The loader SHALL verify the magic number "TENS" (0x54454E53) at file offset 0.

**Verification:** Unit test with valid/invalid magic numbers.

#### REQ-LOAD-002: Version Validation
The loader SHALL verify format version equals 1.

**Verification:** Unit test with version 0, 1, 2.

#### REQ-LOAD-003: Data Type Validation
The loader SHALL verify dtype equals CT_DTYPE_Q16_16 (0).

**Verification:** Unit test with dtype 0, 1.

#### REQ-LOAD-004: Dimension Parsing
The loader SHALL read ndims and dims[0..ndims-1] as little-endian uint32.

**Verification:** Unit test with 1D, 2D, 3D, 4D tensors.

#### REQ-LOAD-005: Dimension Limit
The loader SHALL reject files with ndims > CT_MAX_DIMS (4) and set FAULT_FORMAT.

**Verification:** Unit test with ndims = 5.

#### REQ-LOAD-006: Data Reading
The loader SHALL read data values as little-endian int32 (Q16.16).

**Verification:** Bit-identity test across platforms.

#### REQ-LOAD-007: Buffer Bounds
The loader SHALL verify the provided buffer is large enough for total_elements × sizeof(fixed_t) and set FAULT_IO if insufficient.

**Verification:** Unit test with undersized buffer.

#### REQ-LOAD-008: Total Elements Validation
The loader SHALL compute total_elements as the product of dims[] and verify it matches any stored value.

**Verification:** Unit test with mismatched total_elements.

---

### 4.2 CSV File Loading

#### REQ-LOAD-010: CSV Parsing
The loader SHALL parse CSV files with comma-separated decimal values.

**Verification:** Unit test with sample CSV files.

#### REQ-LOAD-011: Decimal Conversion
The loader SHALL convert decimal strings to Q16.16 using the integer-only algorithm specified in CT-MATH-001 §12.2.

**Verification:** Test vectors from CT-MATH-001 §12.3.

#### REQ-LOAD-012: Sign Handling
The loader SHALL correctly parse negative values prefixed with '-'.

**Verification:** Unit test with positive and negative values.

#### REQ-LOAD-013: Fractional Precision
The loader SHALL parse up to CT_MAX_FRAC_DIGITS (16) fractional digits.

**Verification:** Unit test with varying fractional lengths.

#### REQ-LOAD-014: Overflow Handling
The loader SHALL set FAULT_OVERFLOW if the converted value exceeds FIXED_MAX and return FIXED_MAX.

**Verification:** Unit test with "32768.0".

#### REQ-LOAD-015: Underflow Handling
The loader SHALL set FAULT_UNDERFLOW if the converted value is below FIXED_MIN and return FIXED_MIN.

**Verification:** Unit test with "-32769.0".

#### REQ-LOAD-016: Format Error Handling
The loader SHALL set FAULT_FORMAT on malformed input (invalid characters, multiple decimal points).

**Verification:** Unit test with "1.2.3", "abc", "1e5".

#### REQ-LOAD-017: Whitespace Handling
The loader SHALL trim leading and trailing whitespace from values.

**Verification:** Unit test with " 1.5 ", "\t2.0\n".

#### REQ-LOAD-018: Empty Value Handling
The loader SHALL set FAULT_FORMAT on empty values between commas.

**Verification:** Unit test with "1,,3".

---

### 4.3 Statistics File Loading

#### REQ-LOAD-020: Statistics Magic Validation
The loader SHALL verify the magic number "STAT" (0x53544154).

**Verification:** Unit test with valid/invalid magic.

#### REQ-LOAD-021: Statistics Version Validation
The loader SHALL verify format version equals 1.

**Verification:** Unit test.

#### REQ-LOAD-022: Channel Count Validation
The loader SHALL verify num_channels ≤ CT_MAX_CHANNELS (16) and set FAULT_FORMAT otherwise.

**Verification:** Unit test with num_channels = 17.

#### REQ-LOAD-023: Statistics Reading
The loader SHALL read mean and inv_std for each channel as little-endian int32 (Q16.16).

**Verification:** Unit test with known values.

---

### 4.4 Dataset Hashing

#### REQ-LOAD-030: Dataset Hash Computation
The loader SHALL compute the dataset hash as SHA-256 over the serialized dataset (header + all data).

**Verification:** Test with known dataset, verify hash.

#### REQ-LOAD-031: Deterministic Hashing
The same dataset loaded on different platforms SHALL produce identical hashes.

**Verification:** Cross-platform bit-identity test.

---

### 4.5 Error Handling

#### REQ-LOAD-040: File Open Failure
The loader SHALL set FAULT_IO if the file cannot be opened.

**Verification:** Unit test with non-existent file.

#### REQ-LOAD-041: Read Failure
The loader SHALL set FAULT_IO if a read operation fails or returns fewer bytes than expected.

**Verification:** Unit test with truncated file.

#### REQ-LOAD-042: Fault Stickiness
Fault flags SHALL remain set until explicitly cleared.

**Verification:** Unit test verifying flags persist across operations.

---

### 4.6 Memory Safety

#### REQ-LOAD-050: No Dynamic Allocation
The loader SHALL NOT call malloc, calloc, realloc, or free.

**Verification:** Static analysis, code review.

#### REQ-LOAD-051: Caller-Provided Buffers
All data buffers SHALL be provided by the caller.

**Verification:** API review.

#### REQ-LOAD-052: Bounds Checking
The loader SHALL verify all buffer accesses are within provided bounds.

**Verification:** Unit tests with boundary conditions.

---

## 5. Interfaces

### 5.1 Functions

```c
/**
 * @brief Load binary tensor file.
 * @traceability REQ-LOAD-001 through REQ-LOAD-008
 */
int ct_load_tensor(const char *path, ct_dataset_t *dataset,
                   fixed_t *data_buf, uint32_t buf_size,
                   ct_fault_flags_t *faults);

/**
 * @brief Load CSV file with decimal conversion.
 * @traceability REQ-LOAD-010 through REQ-LOAD-018
 */
int ct_load_csv(const char *path, ct_dataset_t *dataset,
                fixed_t *data_buf, uint32_t buf_size,
                uint32_t expected_cols,
                ct_fault_flags_t *faults);

/**
 * @brief Load statistics file.
 * @traceability REQ-LOAD-020 through REQ-LOAD-023
 */
int ct_load_stats(const char *path, ct_norm_config_t *config,
                  ct_fault_flags_t *faults);

/**
 * @brief Convert decimal string to Q16.16.
 * @traceability REQ-LOAD-011 through REQ-LOAD-018
 */
fixed_t ct_decimal_to_fixed(const char *str, uint32_t len,
                            ct_fault_flags_t *faults);

/**
 * @brief Compute dataset hash.
 * @traceability REQ-LOAD-030, REQ-LOAD-031
 */
void ct_hash_dataset(const ct_dataset_t *dataset, ct_hash_t out);
```

### 5.2 Return Values

| Value | Meaning |
|-------|---------|
| 0 | Success |
| -1 | Failure (check fault flags) |

---

## 6. Test Cases

| Test ID | Requirement | Description |
|---------|-------------|-------------|
| T-LOAD-001 | REQ-LOAD-001 | Valid magic "TENS" |
| T-LOAD-002 | REQ-LOAD-001 | Invalid magic "XXXX" |
| T-LOAD-003 | REQ-LOAD-006 | Load 1D tensor, verify values |
| T-LOAD-004 | REQ-LOAD-006 | Load 3D tensor (e.g., 3×28×28 image) |
| T-LOAD-005 | REQ-LOAD-011 | Decimal "123.456" → 8090542 |
| T-LOAD-006 | REQ-LOAD-011 | Decimal "0.5" → 32768 |
| T-LOAD-007 | REQ-LOAD-011 | Decimal "-1.5" → -98304 |
| T-LOAD-008 | REQ-LOAD-014 | Overflow "40000.0" → FIXED_MAX + fault |
| T-LOAD-009 | REQ-LOAD-016 | Malformed "1.2.3" → fault |
| T-LOAD-010 | REQ-LOAD-031 | Cross-platform hash identity |

---

## 7. Traceability Matrix

| Requirement | CT-MATH-001 | CT-STRUCT-001 | Test |
|-------------|-------------|---------------|------|
| REQ-LOAD-001 | §9.2 | §14.1 | T-LOAD-001, T-LOAD-002 |
| REQ-LOAD-006 | §9.2 | §2.1 | T-LOAD-003, T-LOAD-004 |
| REQ-LOAD-011 | §12.2 | §15 | T-LOAD-005, T-LOAD-006, T-LOAD-007 |
| REQ-LOAD-014 | §12.2 | §3 | T-LOAD-008 |
| REQ-LOAD-031 | §14.1 | §4 | T-LOAD-010 |

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial release |
