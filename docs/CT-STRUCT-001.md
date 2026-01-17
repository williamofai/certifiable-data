# CT-STRUCT-001: Data Structure Specification

**Project:** Certifiable Data  
**Version:** 2.0.1  
**Status:** Final  
**Author:** William Murray  
**Date:** 2026-01-16

---

## 0. Normative Reference

This document derives all structures from **CT-MATH-001 v2.0.0**. Every field, type, and constant is a direct transcription of the mathematical specification.

**Alignment Principle:** If CT-MATH-001 specifies a value, this document uses that exact value. No independent design decisions are made here.

---

## 1. Purpose

This document defines all data structures used in certifiable-data. Structures are organized by their corresponding CT-MATH-001 section.

---

## 2. Core Types (CT-MATH-001 §2)

### 2.1 Fixed-Point Types

```c
/** @brief Q16.16 signed fixed-point (CT-MATH-001 §2.1) */
typedef int32_t fixed_t;

/** @brief 64-bit accumulator for intermediate calculations (CT-MATH-001 §2.4) */
typedef int64_t acc_t;
```

### 2.2 Fixed-Point Constants

```c
/** @traceability CT-MATH-001 §2.3 */
#define FIXED_SHIFT   16
#define FIXED_ONE     65536               /* 0x00010000 */
#define FIXED_HALF    32768               /* 0x00008000 */
#define FIXED_ZERO    0                   /* 0x00000000 */
#define FIXED_MAX     2147483647          /* 0x7FFFFFFF */
#define FIXED_MIN     (-2147483647 - 1)   /* 0x80000000 */
#define FIXED_EPS     1                   /* 0x00000001 */
```

**Note:** `FIXED_MIN` is written as `(-2147483647 - 1)` to avoid integer literal overflow in C99.

### 2.3 Compile-Time Limits

```c
#define CT_MAX_DIMS         4     /**< Maximum tensor dimensions */
#define CT_MAX_CHANNELS     16    /**< Maximum normalisation channels */
#define CT_MAX_BATCH_SIZE   256   /**< Maximum samples per batch */
#define CT_MAX_FRAC_DIGITS  16    /**< Maximum fractional digits in decimal parsing */
#define CT_CONFIG_VERSION   1     /**< Current configuration format version */
```

---

## 3. Fault Flags (CT-MATH-001 §3.1, §13)

**Portability Note:** This specification assumes standard little-endian bitfield allocation (LSB first). For maximum portability across compilers, implementations MAY replace bitfields with a `uint32_t` and explicit bit masks.

```c
/**
 * @brief Sticky fault flags for error tracking.
 * @traceability CT-MATH-001 §3.1, §13.1
 */
typedef struct {
    uint32_t overflow     : 1;   /**< Arithmetic saturated to MAX */
    uint32_t underflow    : 1;   /**< Arithmetic saturated to MIN */
    uint32_t div_zero     : 1;   /**< Division by zero attempted */
    uint32_t domain       : 1;   /**< Invalid input domain (e.g., shift > 62) */
    uint32_t io_error     : 1;   /**< File I/O failure */
    uint32_t format_error : 1;   /**< Invalid file/string format */
    uint32_t hash_mismatch: 1;   /**< Hash verification failed */
    uint32_t _reserved    : 25;
} ct_fault_flags_t;

/**
 * @brief Check if any fault flag is set.
 * @traceability CT-MATH-001 §13.2
 */
static inline bool ct_has_fault(const ct_fault_flags_t *f) {
    return f->overflow || f->underflow || f->div_zero || f->domain ||
           f->io_error || f->format_error || f->hash_mismatch;
}

/**
 * @brief Clear all fault flags.
 */
static inline void ct_clear_faults(ct_fault_flags_t *f) {
    f->overflow = 0;
    f->underflow = 0;
    f->div_zero = 0;
    f->domain = 0;
    f->io_error = 0;
    f->format_error = 0;
    f->hash_mismatch = 0;
    f->_reserved = 0;
}
```

---

## 4. Hash Type (CT-MATH-001 §10)

```c
/**
 * @brief SHA-256 digest (32 bytes).
 * @traceability CT-MATH-001 §10
 */
typedef uint8_t ct_hash_t[32];

/**
 * @brief Domain separation prefixes for Merkle hashing.
 * @traceability CT-MATH-001 §10.1
 */
#define CT_DOMAIN_LEAF       0x00
#define CT_DOMAIN_INTERNAL   0x01
#define CT_DOMAIN_BATCH      0x02
#define CT_DOMAIN_EPOCH      0x03
#define CT_DOMAIN_PROVENANCE 0x04
```

---

## 5. Tensor Shape (CT-MATH-001 §11.2)

```c
/**
 * @brief Tensor shape descriptor.
 * @traceability CT-MATH-001 §11.2
 * 
 * Invariant: total_elements MUST equal the product of dims[0..ndims-1].
 * Implementations SHALL validate this at load time and set FAULT_FORMAT on mismatch.
 */
typedef struct {
    uint8_t  version;                   /**< Format version (must be 1) */
    uint8_t  dtype;                     /**< 0 = Q16.16 */
    uint8_t  ndims;                     /**< Number of dimensions [0, CT_MAX_DIMS] */
    uint8_t  _pad;                      /**< Padding for alignment */
    uint32_t dims[CT_MAX_DIMS];         /**< Dimension sizes */
    uint32_t total_elements;            /**< Product of dims (precomputed) */
} ct_shape_t;

/**
 * @brief Data type identifiers.
 * @traceability CT-MATH-001 §11.2
 */
#define CT_DTYPE_Q16_16  0
```

---

## 6. Normalisation Structures (CT-MATH-001 §4)

### 6.1 Channel Statistics

```c
/**
 * @brief Per-channel precomputed statistics.
 * @traceability CT-MATH-001 §4.3
 * 
 * Statistics are computed OFFLINE and provided at initialisation.
 * Runtime computation is forbidden.
 */
typedef struct {
    fixed_t mean;       /**< Precomputed mean μ (Q16.16) */
    fixed_t inv_std;    /**< Precomputed 1/σ (Q16.16) */
} ct_channel_stats_t;
```

### 6.2 Normalisation Configuration

```c
/**
 * @brief Normalisation configuration for all channels.
 * @traceability CT-MATH-001 §4.4
 */
typedef struct {
    uint32_t num_channels;                          /**< C: number of channels */
    ct_channel_stats_t stats[CT_MAX_CHANNELS];      /**< Per-channel stats */
} ct_norm_config_t;
```

---

## 7. PRNG Structures (CT-MATH-001 §5)

### 7.1 Augmentation IDs

```c
/**
 * @brief Augmentation operation identifiers for PRNG.
 * @traceability CT-MATH-001 §5.2
 */
typedef enum {
    CT_AUG_RESERVED    = 0x00,
    CT_AUG_HFLIP       = 0x01,
    CT_AUG_VFLIP       = 0x02,
    CT_AUG_CROP_Y      = 0x03,
    CT_AUG_CROP_X      = 0x04,
    CT_AUG_BRIGHTNESS  = 0x05,
    CT_AUG_NOISE       = 0x06
} ct_augment_id_t;
```

### 7.2 PRNG is Stateless

Per CT-MATH-001 §5.1, the PRNG is a **pure function** with no internal state:

```c
/**
 * @brief Pure PRNG function (no state structure needed).
 * @traceability CT-MATH-001 §5.3
 * 
 * @param seed    Master seed (64-bit)
 * @param op_id   Operation identifier (64-bit, see §5.2)
 * @param step    Step counter within operation
 * @return        Deterministic 32-bit output
 */
uint32_t ct_prng(uint64_t seed, uint64_t op_id, uint64_t step);
```

---

## 8. Augmentation Structures (CT-MATH-001 §8)

### 8.1 Augmentation Flags

```c
/**
 * @brief Augmentation enable flags.
 * @traceability CT-MATH-001 §8.1
 * 
 * Pipeline order is fixed: crop → hflip → vflip → brightness → noise
 */
typedef struct {
    uint32_t enable_crop       : 1;   /**< Random crop */
    uint32_t enable_hflip      : 1;   /**< Horizontal flip */
    uint32_t enable_vflip      : 1;   /**< Vertical flip */
    uint32_t enable_brightness : 1;   /**< Brightness adjustment */
    uint32_t enable_noise      : 1;   /**< Additive noise */
    uint32_t _reserved         : 27;
} ct_augment_flags_t;
```

### 8.2 Crop Configuration

```c
/**
 * @brief Random crop parameters.
 * @traceability CT-MATH-001 §8.4
 */
typedef struct {
    uint32_t crop_height;   /**< Output height after crop */
    uint32_t crop_width;    /**< Output width after crop */
} ct_crop_config_t;
```

### 8.3 Augmentation Configuration

```c
/**
 * @brief Complete augmentation configuration.
 * @traceability CT-MATH-001 §8
 */
typedef struct {
    ct_augment_flags_t flags;       /**< Which augmentations enabled */
    ct_crop_config_t   crop;        /**< Crop parameters */
    fixed_t brightness_delta;       /**< Max brightness change δ (Q16.16) */
    fixed_t noise_amplitude;        /**< Noise amplitude (Q16.16) */
} ct_augment_config_t;
```

---

## 9. Shuffling Structures (CT-MATH-001 §7)

### 9.1 Permutation Parameters

```c
/**
 * @brief Feistel permutation parameters (precomputed).
 * @traceability CT-MATH-001 §7.2
 * 
 * Note: Uses balanced Feistel split where both halves have the same width.
 * half_bits = (k + 1) / 2, which matches CT-MATH-001 v2.0.0 §7.2.
 */
typedef struct {
    uint64_t seed;          /**< Master seed */
    uint32_t epoch;         /**< Current epoch */
    uint32_t N;             /**< Dataset size */
    uint32_t k;             /**< ceil_log2(N) */
    uint32_t half_bits;     /**< (k + 1) / 2 — balanced split */
    uint32_t half_mask;     /**< (1 << half_bits) - 1 */
    uint32_t range;         /**< 1 << k */
} ct_permute_params_t;

/**
 * @brief Initialize permutation parameters.
 * @traceability CT-MATH-001 §7.2, §7.4
 */
void ct_permute_init(ct_permute_params_t *p, uint64_t seed, uint32_t epoch, uint32_t N);

/**
 * @brief Compute permuted index.
 * @traceability CT-MATH-001 §7.2
 * 
 * @param p       Permutation parameters
 * @param index   Input index [0, N-1]
 * @param faults  Fault flags (set on bounded loop exhaustion)
 * @return        Permuted index [0, N-1]
 */
uint32_t ct_permute_index(const ct_permute_params_t *p, uint32_t index, ct_fault_flags_t *faults);
```

---

## 10. Batch Structures (CT-MATH-001 §9)

### 10.1 Sample Reference

```c
/**
 * @brief Reference to a sample in the dataset.
 * @traceability CT-MATH-001 §9.1
 */
typedef struct {
    uint32_t original_index;    /**< Index in sequential order: b × B + i */
    uint32_t shuffled_index;    /**< Index after permutation: π_e(original) */
} ct_sample_ref_t;
```

### 10.2 Batch Descriptor

```c
/**
 * @brief Batch descriptor with Merkle commitment.
 * @traceability CT-MATH-001 §9.1, §10.5
 */
typedef struct {
    uint32_t epoch;                                 /**< Epoch number */
    uint32_t batch_index;                           /**< Batch index within epoch */
    uint32_t batch_size;                            /**< Actual samples in batch [1, B] */
    ct_sample_ref_t refs[CT_MAX_BATCH_SIZE];        /**< Sample references */
    ct_hash_t merkle_root;                          /**< Merkle root of sample hashes */
    ct_hash_t batch_hash;                           /**< H_batch (includes metadata) */
} ct_batch_t;
```

---

## 11. Dataset Structures (CT-MATH-001 §11)

### 11.1 Sample

```c
/**
 * @brief Single sample with shape and data.
 * @traceability CT-MATH-001 §11.2
 * 
 * Data buffer is caller-provided (no dynamic allocation).
 * Data MUST be contiguous Q16.16 values in row-major order.
 */
typedef struct {
    ct_shape_t shape;       /**< Sample shape (includes dtype, version) */
    fixed_t   *data;        /**< Pointer to Q16.16 data (non-owning) */
} ct_sample_t;
```

### 11.2 Dataset Descriptor

```c
/**
 * @brief Dataset descriptor.
 * @traceability CT-MATH-001 §11
 */
typedef struct {
    uint32_t    num_samples;        /**< N: total samples */
    ct_shape_t  sample_shape;       /**< Shape of each sample (uniform) */
    fixed_t    *data;               /**< Contiguous data buffer (non-owning) */
    ct_hash_t   dataset_hash;       /**< SHA-256 of entire dataset */
} ct_dataset_t;
```

---

## 12. Provenance Structures (CT-MATH-001 §10.7)

```c
/**
 * @brief Dataset provenance chain state.
 * @traceability CT-MATH-001 §10.7
 */
typedef struct {
    ct_hash_t dataset_hash;     /**< SHA-256 of dataset */
    ct_hash_t config_hash;      /**< SHA-256 of serialized config */
    uint64_t  seed;             /**< Master seed */
    uint32_t  current_epoch;    /**< Current epoch */
    ct_hash_t prev_hash;        /**< h_{e-1} */
    ct_hash_t current_hash;     /**< h_e */
} ct_provenance_t;
```

---

## 13. Pipeline Configuration (CT-MATH-001 §11.4)

### 13.1 Configuration Structure

```c
/**
 * @brief Complete pipeline configuration.
 * @traceability CT-MATH-001 §11.4
 */
typedef struct {
    uint8_t  version;               /**< Config version (must be 1) */
    uint8_t  _pad[3];               /**< Padding for alignment */
    uint32_t batch_size;            /**< B: samples per batch */
    uint64_t seed;                  /**< Master seed */
    ct_augment_config_t augment;    /**< Augmentation config */
    ct_norm_config_t    norm;       /**< Normalisation config */
} ct_pipeline_config_t;
```

### 13.2 Pipeline Context

```c
/**
 * @brief Runtime pipeline context.
 */
typedef struct {
    ct_pipeline_config_t config;        /**< Configuration (immutable after init) */
    ct_dataset_t        *dataset;       /**< Dataset pointer (non-owning) */
    ct_permute_params_t  permute;       /**< Permutation parameters */
    ct_provenance_t      provenance;    /**< Provenance chain state */
    uint32_t current_epoch;             /**< Current epoch */
    uint32_t current_batch;             /**< Current batch index in epoch */
    uint32_t batches_per_epoch;         /**< ceil(N / B) */
    ct_fault_flags_t faults;            /**< Sticky fault flags */
} ct_pipeline_ctx_t;
```

---

## 14. Serialization Headers (CT-MATH-001 §11)

### 14.1 Tensor File Header

```c
/**
 * @brief Binary tensor file header.
 * @traceability CT-MATH-001 §11.2
 * 
 * All fields little-endian.
 */
typedef struct {
    uint8_t  magic[4];              /**< "TENS" = {0x54, 0x45, 0x4E, 0x53} */
    uint8_t  version;               /**< Format version (1) */
    uint8_t  dtype;                 /**< CT_DTYPE_Q16_16 = 0 */
    uint8_t  ndims;                 /**< Number of dimensions */
    uint8_t  _pad;                  /**< Padding */
    uint32_t dims[CT_MAX_DIMS];     /**< Dimension sizes (little-endian) */
} ct_tensor_file_header_t;

#define CT_TENSOR_MAGIC  "TENS"
```

### 14.2 Statistics File Header

```c
/**
 * @brief Statistics file header.
 * @traceability CT-MATH-001 §11.3
 */
typedef struct {
    uint8_t  magic[4];              /**< "STAT" = {0x53, 0x54, 0x41, 0x54} */
    uint8_t  version;               /**< Format version (1) */
    uint8_t  num_channels;          /**< Number of channels */
    uint8_t  _pad[2];               /**< Padding */
} ct_stats_file_header_t;

#define CT_STATS_MAGIC  "STAT"
```

---

## 15. Decimal Parsing Context (CT-MATH-001 §12)

```c
/**
 * @brief Intermediate state for decimal→fixed conversion.
 * @traceability CT-MATH-001 §12.2
 * 
 * Used internally by ct_decimal_to_fixed().
 */
typedef struct {
    int32_t  sign;                              /**< +1 or -1 */
    uint64_t integer_part;                      /**< Integer portion */
    uint8_t  frac_digits[CT_MAX_FRAC_DIGITS];   /**< Fractional digits */
    uint32_t frac_len;                          /**< Number of fractional digits */
} ct_decimal_parse_t;
```

---

## 16. Function Signatures

### 16.1 DVM Primitives (CT-MATH-001 §3)

```c
/** @traceability CT-MATH-001 §3.2 */
int32_t dvm_clamp32(int64_t x, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §3.3 */
int32_t dvm_add32(int32_t a, int32_t b, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §3.4 */
int32_t dvm_sub32(int32_t a, int32_t b, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §3.5 */
int64_t dvm_mul64(int32_t a, int32_t b);

/** @traceability CT-MATH-001 §3.6 */
int32_t dvm_round_shift_rne(int64_t x, uint32_t shift, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §3.7 */
fixed_t dvm_mul_q16(fixed_t a, fixed_t b, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §3.8 */
fixed_t dvm_div_q16(fixed_t a, fixed_t b, ct_fault_flags_t *faults);
```

### 16.2 Normalisation (CT-MATH-001 §4)

```c
/** @traceability CT-MATH-001 §4.2 */
fixed_t ct_normalize(fixed_t x, fixed_t mean, fixed_t inv_std, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §4.4 */
void ct_normalize_sample(const ct_sample_t *in, ct_sample_t *out,
                         const ct_norm_config_t *config, ct_fault_flags_t *faults);
```

### 16.3 PRNG (CT-MATH-001 §5)

```c
/** @traceability CT-MATH-001 §5.3 */
uint32_t ct_prng(uint64_t seed, uint64_t op_id, uint64_t step);

/** @traceability CT-MATH-001 §5.2 */
uint64_t ct_make_op_id(const ct_hash_t dataset_hash, uint32_t epoch,
                       uint32_t sample_idx, uint8_t augment_id, uint8_t element_idx_lo);
```

### 16.4 Rejection Sampling (CT-MATH-001 §6)

```c
/** @traceability CT-MATH-001 §6.2 */
uint32_t ct_unbiased_random(uint64_t seed, uint64_t op_id, uint32_t range,
                            ct_fault_flags_t *faults);
```

### 16.5 Permutation (CT-MATH-001 §7)

```c
/** @traceability CT-MATH-001 §7.4 */
uint32_t ct_ceil_log2(uint32_t n);

/** @traceability CT-MATH-001 §7.2 */
void ct_permute_init(ct_permute_params_t *p, uint64_t seed, uint32_t epoch, uint32_t N);

/** @traceability CT-MATH-001 §7.2 */
uint32_t ct_permute_index(const ct_permute_params_t *p, uint32_t index,
                          ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §7.3 */
uint32_t ct_feistel_round(uint32_t R, uint64_t seed, uint32_t epoch, uint8_t round);
```

### 16.6 Augmentation (CT-MATH-001 §8)

```c
/** @traceability CT-MATH-001 §8.2 */
void ct_augment_hflip(const ct_sample_t *in, ct_sample_t *out,
                      uint64_t seed, uint64_t op_id, bool enabled);

/** @traceability CT-MATH-001 §8.3 */
void ct_augment_vflip(const ct_sample_t *in, ct_sample_t *out,
                      uint64_t seed, uint64_t op_id, bool enabled);

/** @traceability CT-MATH-001 §8.4 */
void ct_augment_crop(const ct_sample_t *in, ct_sample_t *out,
                     const ct_crop_config_t *crop,
                     uint64_t seed, uint64_t op_id_y, uint64_t op_id_x,
                     bool enabled, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §8.5 */
void ct_augment_brightness(const ct_sample_t *in, ct_sample_t *out,
                           fixed_t delta, uint64_t seed, uint64_t op_id,
                           bool enabled, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §8.6 */
void ct_augment_noise(const ct_sample_t *in, ct_sample_t *out,
                      fixed_t amplitude, uint64_t seed, uint64_t op_id,
                      bool enabled, ct_fault_flags_t *faults);

/** @traceability CT-MATH-001 §8.1 */
void ct_augment_pipeline(const ct_sample_t *in, ct_sample_t *out,
                         const ct_augment_config_t *config,
                         uint64_t seed, const ct_hash_t dataset_hash,
                         uint32_t epoch, uint32_t sample_idx,
                         ct_fault_flags_t *faults);
```

### 16.7 Batching (CT-MATH-001 §9)

```c
/** @traceability CT-MATH-001 §9.1 */
void ct_batch_construct(ct_batch_t *batch, const ct_dataset_t *dataset,
                        const ct_permute_params_t *permute,
                        uint32_t batch_size, uint32_t batch_idx,
                        ct_fault_flags_t *faults);
```

### 16.8 Merkle (CT-MATH-001 §10)

```c
/** @traceability CT-MATH-001 §10.2 */
void ct_hash_sample(const ct_sample_t *sample, ct_hash_t out);

/** @traceability CT-MATH-001 §10.3 */
void ct_hash_node(const ct_hash_t left, const ct_hash_t right, ct_hash_t out);

/** @traceability CT-MATH-001 §10.4 */
void ct_merkle_root(const ct_hash_t *leaves, uint32_t count, ct_hash_t out);

/** @traceability CT-MATH-001 §10.5 */
void ct_hash_batch(const ct_batch_t *batch, ct_hash_t out);

/** @traceability CT-MATH-001 §10.6 */
void ct_hash_epoch(const ct_hash_t *batch_hashes, uint32_t num_batches,
                   uint32_t epoch, ct_hash_t out);

/** @traceability CT-MATH-001 §10.7 */
void ct_provenance_init(ct_provenance_t *prov, const ct_hash_t dataset_hash,
                        const ct_hash_t config_hash, uint64_t seed);

/** @traceability CT-MATH-001 §10.7 */
void ct_provenance_advance(ct_provenance_t *prov, const ct_hash_t epoch_hash);
```

### 16.9 Serialization (CT-MATH-001 §11)

```c
/** @traceability CT-MATH-001 §11.2 */
uint32_t ct_serialize_sample(const ct_sample_t *sample, uint8_t *buf, uint32_t buf_size);

/** @traceability CT-MATH-001 §11.3 */
uint32_t ct_serialize_stats(const ct_norm_config_t *stats, uint8_t *buf, uint32_t buf_size);

/** @traceability CT-MATH-001 §11.4 */
uint32_t ct_serialize_config(const ct_pipeline_config_t *config, uint8_t *buf, uint32_t buf_size);
```

### 16.10 Decimal Conversion (CT-MATH-001 §12)

```c
/** @traceability CT-MATH-001 §12.2 */
fixed_t ct_decimal_to_fixed(const char *str, uint32_t len, ct_fault_flags_t *faults);
```

### 16.11 Loading

```c
/** @brief Load tensor file into dataset. */
int ct_load_tensor(const char *path, ct_dataset_t *dataset,
                   fixed_t *data_buf, uint32_t buf_size,
                   ct_fault_flags_t *faults);

/** @brief Load statistics file. */
int ct_load_stats(const char *path, ct_norm_config_t *config,
                  ct_fault_flags_t *faults);

/** @brief Load CSV file into dataset. */
int ct_load_csv(const char *path, ct_dataset_t *dataset,
                fixed_t *data_buf, uint32_t buf_size,
                ct_fault_flags_t *faults);
```

---

## 17. Alignment Matrix

| CT-MATH-001 Section | Structure(s) | Function(s) |
|---------------------|--------------|-------------|
| §2 Fixed-point | `fixed_t`, constants | — |
| §3 DVM primitives | `ct_fault_flags_t` | `dvm_*` |
| §4 Normalisation | `ct_channel_stats_t`, `ct_norm_config_t` | `ct_normalize*` |
| §5 PRNG | `ct_augment_id_t` | `ct_prng`, `ct_make_op_id` |
| §6 Rejection sampling | — | `ct_unbiased_random` |
| §7 Feistel | `ct_permute_params_t` | `ct_permute_*`, `ct_feistel_round` |
| §8 Augmentation | `ct_augment_*` | `ct_augment_*` |
| §9 Batching | `ct_sample_ref_t`, `ct_batch_t` | `ct_batch_construct` |
| §10 Merkle | `ct_hash_t`, `ct_provenance_t` | `ct_hash_*`, `ct_merkle_*`, `ct_provenance_*` |
| §11 Serialization | `ct_shape_t`, `ct_*_header_t` | `ct_serialize_*` |
| §12 Decimal conversion | `ct_decimal_parse_t` | `ct_decimal_to_fixed` |
| §13 Faults | `ct_fault_flags_t` | `ct_has_fault`, `ct_clear_faults` |

---

## 18. Memory Model

All structures follow the certifiable framework memory model:

| Rule | Requirement |
|------|-------------|
| **No dynamic allocation** | All buffers provided by caller |
| **Fixed sizes** | Maximum dimensions are compile-time constants |
| **Contiguous storage** | Dataset data in single buffer |
| **Non-owning pointers** | Structures hold references, not ownership |
| **Explicit padding** | All padding bytes are named `_pad` |
| **No hidden state** | PRNG is pure function; all state is explicit |

---

## 19. Size Guarantees

```c
/* Compile-time size assertions (example) */
_Static_assert(sizeof(ct_fault_flags_t) == 4, "fault_flags must be 4 bytes");
_Static_assert(sizeof(ct_hash_t) == 32, "hash must be 32 bytes");
_Static_assert(sizeof(ct_shape_t) == 24, "shape must be 24 bytes");
```

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-01-16 | William Murray | Initial draft |
| 2.0.0 | 2026-01-16 | William Murray | Aligned to CT-MATH-001 v2.0.0 |
| 2.0.1 | 2026-01-16 | William Murray | Tightenings: bitfield portability note, total_elements validation, row-major order note, CT_CONFIG_VERSION macro |

---

## Appendix A: Traceability Summary

Every structure and function in this document maps to a specific section of CT-MATH-001 v2.0.0. The `@traceability` tags in the source code provide bidirectional links for certification audits.

---

## Appendix B: Review Conclusion

**CT-STRUCT-001 v2.0.1 is fully aligned with CT-MATH-001 v2.0.0 and introduces no independent semantics.**

All structures are deterministic, bounded, memory-safe, and suitable for safety-critical, certifiable data pipelines. The document is complete and production-ready.
