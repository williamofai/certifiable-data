# certifiable-data

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/williamofai/certifiable-data)
[![Tests](https://img.shields.io/badge/tests-133%20passing-brightgreen)](https://github.com/williamofai/certifiable-data)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-x86%20%7C%20ARM%20%7C%20RISC--V-lightgrey)](https://github.com/williamofai/certifiable-data)

**Deterministic, bit-perfect data pipeline for safety-critical ML systems.**

Pure C99. Zero dynamic allocation. Certifiable for DO-178C, IEC 62304, and ISO 26262.

---

## The Problem

Standard ML data pipelines are inherently non-deterministic:
- Floating-point normalization varies across platforms
- Random shuffling produces different orders each run
- Data augmentation depends on random number generators
- Non-deterministic batch construction

For safety-critical systems, you cannot certify what you cannot reproduce.

**Read more:**
- [Why Floating Point Is Dangerous](https://speytech.com/ai-architecture/floating-point-danger/)
- [Bit-Perfect Reproducibility: Why It Matters and How to Prove It](https://speytech.com/insights/bit-perfect-reproducibility/)
- [Why Your ML Model Gives Different Results Every Tuesday](https://speytech.com/insights/ml-nondeterminism-problem/)

## The Solution

`certifiable-data` defines data processing as a **deterministic transformation pipeline**:

### 1. Fixed-Point Arithmetic
Q16.16 format for all normalization. Same math, same result, every platform.

**Read more:** [Fixed-Point Neural Networks: The Math Behind Q16.16](https://speytech.com/insights/fixed-point-neural-networks/)

### 2. Deterministic Shuffling
Feistel network with cycle-walking. Cryptographic bijection: `π: [0, N-1] → [0, N-1]`.

### 3. Reproducible Augmentation
Counter-based PRNG: `PRNG(seed, sample_id, epoch) → deterministic transforms`. Same seed = same augmentation.

### 4. Merkle Audit Trail
Every batch cryptographically committed. Any batch verifiable in O(log N) time.

**Read more:** [Cryptographic Execution Tracing and Evidentiary Integrity](https://speytech.com/insights/cryptographic-proof-execution/)

**Result:** `B_t = Pipeline(D, seed, epoch, t)` — Data loading is a pure function.

## Status

**All core modules complete — 8/8 test suites passing (142 tests).**

| Module | Description | Status |
|--------|-------------|--------|
| DVM Primitives | Fixed-point arithmetic with fault detection | ✅ |
| Counter-based PRNG | Deterministic pseudo-random generation | ✅ |
| Feistel Shuffle | Cycle-walking bijection for any N | ✅ |
| Normalization | Q16.16 standardization | ✅ |
| Augmentation | Deterministic flip, crop, noise | ✅ |
| Batch Construction | Static allocation with Merkle commitment | ✅ |
| Merkle Chain | SHA256 provenance trail | ✅ |
| Bit Identity | Cross-platform reproducibility tests | ✅ |

## Quick Start

### Build
```bash
mkdir build && cd build
cmake ..
make
make test  # Run all 8 test suites (142 tests)
```

### Expected Output
```
100% tests passed, 0 tests failed out of 8
Total Test time (real) = 0.04 sec
```

### Basic Data Pipeline
```c
#include "ct_types.h"
#include "loader.h"
#include "normalize.h"
#include "augment.h"
#include "shuffle.h"
#include "batch.h"
#include "merkle.h"

// All buffers pre-allocated (no malloc)
ct_sample_t dataset_samples[60000];
ct_dataset_t dataset = {
    .samples = dataset_samples,
    .num_samples = 60000
};

ct_fault_flags_t faults = {0};

// Load data from CSV (deterministic decimal parsing)
ct_load_csv("mnist.csv", &dataset, &faults);

// Setup normalization
int32_t means[784] = {/* computed mean per feature */};
int32_t inv_stds[784] = {/* 1/std per feature in Q16.16 */};
ct_normalize_ctx_t norm_ctx;
ct_normalize_init(&norm_ctx, means, inv_stds, 784);

// Setup augmentation
ct_augment_flags_t aug_flags = {
    .h_flip = 1,
    .random_crop = 1,
    .gaussian_noise = 0
};
ct_augment_ctx_t aug_ctx;
ct_augment_init(&aug_ctx, seed, epoch, aug_flags);

// Setup shuffling
ct_shuffle_ctx_t shuffle_ctx;
ct_shuffle_init(&shuffle_ctx, seed, epoch);

// Create batch
ct_sample_t batch_samples[32];
ct_hash_t batch_hashes[32];
ct_batch_t batch;
ct_batch_init(&batch, batch_samples, batch_hashes, 32);
ct_batch_fill(&batch, &dataset, batch_index, epoch, seed);

// Verify batch integrity
int valid = ct_batch_verify(&batch);

// Initialize provenance chain
ct_provenance_t prov;
ct_hash_t dataset_hash, config_hash;
ct_hash_dataset(&dataset, dataset_hash);
ct_provenance_init(&prov, dataset_hash, config_hash, seed);

// Advance epoch
ct_hash_t epoch_hash;
ct_hash_epoch(batch_hashes, num_batches, epoch_hash);
ct_provenance_advance(&prov, epoch_hash);

if (ct_has_fault(&faults)) {
    // Pipeline invalidated - do not proceed
}
```

## Architecture

### Deterministic Virtual Machine (DVM)

All arithmetic operations use widening and saturation:
```c
// CORRECT: Explicit widening
int64_t wide = (int64_t)a * (int64_t)b;
return dvm_round_shift_rne(wide, 16, &faults);

// FORBIDDEN: Raw overflow
return (a * b) >> 16;  // Undefined behavior
```

**Read more:** [From Proofs to Code: Mathematical Transcription in C](https://speytech.com/insights/mathematical-proofs-to-code/)

### Fixed-Point Formats

| Format | Use Case | Range | Precision |
|--------|----------|-------|-----------|
| Q16.16 | Data values, normalization | ±32768 | 1.5×10⁻⁵ |
| Q16.16 | Augmentation parameters | ±32768 | 1.5×10⁻⁵ |

### Fault Model

Every operation signals faults without silent failure:
```c
typedef struct {
    uint32_t overflow    : 1;  // Saturated high
    uint32_t underflow   : 1;  // Saturated low
    uint32_t div_zero    : 1;  // Division by zero
    uint32_t domain      : 1;  // Invalid input
    uint32_t precision   : 1;  // Precision loss detected
} ct_fault_flags_t;
```

**Read more:** [Closure, Totality, and the Algebra of Safe Systems](https://speytech.com/insights/closure-totality-algebra/)

### Feistel Shuffle

Cycle-walking Feistel provides true bijection for any dataset size N:
```
π: [0, N-1] → [0, N-1]  (one-to-one and onto)

Test vectors (CT-MATH-001 §7.2):
N=100, seed=0x123456789ABCDEF0, index=0 → 26
N=100, seed=0x123456789ABCDEF0, index=99 → 41
N=60000, seed=0xFEDCBA9876543210, index=0 → 26382
```

Same seed + epoch = same shuffle, every time, every platform.

### Merkle Provenance

Every epoch produces a cryptographic commitment:
```
h_0 = SHA256(0x03 || H_dataset || H_config || seed)
h_e = SHA256(0x04 || h_{e-1} || H_epoch || e)
```

Any epoch can be independently verified. If faults occur, the chain is invalidated.

## Test Coverage

| Module | Tests | Coverage |
|--------|-------|----------|
| DVM Primitives | 38 | CT-MATH-001 §3 test vectors |
| PRNG | 13 | Determinism, distribution quality |
| Shuffle | 19 | Bijection, CT-MATH-001 §7.2 vectors |
| Normalize | 13 | Correctness, metadata preservation |
| Augment | 10 | Deterministic transforms |
| Batch | 12 | Construction, verification |
| Merkle | 20 | Hashing, provenance chain |
| Bit-Identity | 17 | Cross-platform verification |

**Total: 142 tests**

## Documentation

- **CT-MATH-001.md** — Mathematical foundations
- **CT-STRUCT-001.md** — Data structure specifications  
- **docs/requirements/** — SRS documents with full traceability

## Related Projects

| Project | Description |
|---------|-------------|
| [certifiable-data](https://github.com/williamofai/certifiable-data) | Deterministic data pipeline |
| [certifiable-training](https://github.com/williamofai/certifiable-training) | Deterministic training engine |
| [certifiable-quant](https://github.com/williamofai/certifiable-quant) | Deterministic quantization |
| [certifiable-deploy](https://github.com/williamofai/certifiable-deploy) | Deterministic model packaging |
| [certifiable-inference](https://github.com/williamofai/certifiable-inference) | Deterministic inference engine |

Together, these projects provide a complete deterministic ML pipeline for safety-critical systems:
```
certifiable-data → certifiable-training → certifiable-quant → certifiable-deploy → certifiable-inference
```

## Why This Matters

### Medical Devices
IEC 62304 Class C requires traceable, reproducible software. Non-deterministic data loading cannot be validated.

**Read more:** [IEC 62304 Class C: What Medical Device Software Actually Requires](https://speytech.com/insights/iec-62304-class-c/)

### Autonomous Vehicles
ISO 26262 ASIL-D demands provable behavior. Data pipelines must be auditable.

**Read more:**
- [ISO 26262 and ASIL-D: The Role of Determinism](https://speytech.com/insights/iso-26262-asil-d-determinism/)
- [NVIDIA's ASIL-B Linux Kernel: The Path to ASIL-D](https://speytech.com/insights/nvidia-asil-determinism/)

### Aerospace
DO-178C Level A requires complete requirements traceability. "We shuffled the data randomly" is not certifiable.

**Read more:** [DO-178C Level A Certification: How Deterministic Execution Can Streamline Certification Effort](https://speytech.com/insights/do178c-certification/)

This is the first ML data pipeline designed from the ground up for safety-critical certification.

## Deep Dives

Want to understand the engineering principles behind certifiable-data?

**Determinism & Reproducibility:**
- [Bit-Perfect Reproducibility: Why It Matters and How to Prove It](https://speytech.com/insights/bit-perfect-reproducibility/)
- [The ML Non-Determinism Problem](https://speytech.com/insights/ml-nondeterminism-problem/)
- [From Proofs to Code: Mathematical Transcription in C](https://speytech.com/insights/mathematical-proofs-to-code/)

**Safety-Critical Foundations:**
- [The Real Cost of Dynamic Memory in Safety-Critical Systems](https://speytech.com/insights/dynamic-memory-safety-critical/)
- [Closure, Totality, and the Algebra of Safe Systems](https://speytech.com/insights/closure-totality-algebra/)

**Production ML Architecture:**
- [A Complete Deterministic ML Pipeline for Safety-Critical Systems](https://speytech.com/ai-architecture/deterministic-ml-pipeline/)

## Compliance Support

This implementation is designed to support certification under:
- **DO-178C** (Aerospace software)
- **IEC 62304** (Medical device software)
- **ISO 26262** (Automotive functional safety)
- **IEC 61508** (Industrial safety systems)

For compliance packages and certification assistance, contact below.

## Contributing

We welcome contributions from systems engineers working in safety-critical domains. See [CONTRIBUTING.md](CONTRIBUTING.md).

**Important:** All contributors must sign a [Contributor License Agreement](CONTRIBUTOR-LICENSE-AGREEMENT.md).

## License

**Dual Licensed:**
- **Open Source:** GNU General Public License v3.0 (GPLv3)
- **Commercial:** Available for proprietary use in safety-critical systems

For commercial licensing and compliance documentation packages, contact below.

## Patent Protection

This implementation is built on the **Murray Deterministic Computing Platform (MDCP)**,
protected by UK Patent **GB2521625.0**.

MDCP defines a deterministic computing architecture for safety-critical systems,
providing:
- Provable execution bounds
- Resource-deterministic operation
- Certification-ready patterns
- Platform-independent behavior

**Read more:** [MDCP vs. Conventional RTOS](https://speytech.com/insights/mdcp-vs-conventional-rtos/)

For commercial licensing inquiries: william@fstopify.com

## About

Built by **SpeyTech** in the Scottish Highlands.

30 years of UNIX infrastructure experience applied to deterministic computing for safety-critical systems.

Patent: UK GB2521625.0 - Murray Deterministic Computing Platform (MDCP)

**Contact:**
William Murray  
william@fstopify.com  
[speytech.com](https://speytech.com)

**More from SpeyTech:**
- [Technical Articles](https://speytech.com/ai-architecture/)
- [Open Source Projects](https://speytech.com/open-source/)

---

*Building deterministic AI systems for when lives depend on the answer.*

Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
