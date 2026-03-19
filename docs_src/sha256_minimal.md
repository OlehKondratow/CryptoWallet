\page sha256_minimal "sha256_minimal: compact SHA-256 (no trezor-crypto)"
\related sha256_minimal

# `sha256_minimal.c` + `sha256_minimal.h`

<brief>The `sha256_minimal` module is a compact SHA-256 implementation (public domain), used when `USE_CRYPTO_SIGN=0`, when trezor-crypto is not linked but hashing is needed for UI/logging.</brief>

## Overview

<brief>The `sha256_minimal` module is a compact SHA-256 implementation (public domain), used when `USE_CRYPTO_SIGN=0`, when trezor-crypto is not linked but hashing is needed for UI/logging.</brief>

## Abstract (Logic Synthesis)

`sha256_minimal` is a self-contained SHA-256 implementation, independent of everything, allowing a minimal build (without trezor-crypto) to have at least basic hashing for debugging and UI purposes. This is **not** intended for production signing (since there's no ECDSA), but rather for simulation/learning. Business role: provide a fallback when stripping the crypto layer.

## Logic Flow

1. **Initialization:** standard SHA-256 H-values into an array `state[8]`
2. **Main loop:** process data in 64-byte blocks
3. **Padding:** add flag 0x80, zeros, and length in bits (per SHA-256 spec)
4. **Transform:** for each 64-byte block:
   - Unroll 16 words into W[0..63] via SIG0/SIG1 transformations
   - 64 rounds with rotators and majority logic (CH, MAJ, EP0, EP1)
5. **Finalize:** output 32 bytes of state in big-endian

## Interrupts/Registers

No ISRs or registers: pure computation.

## Timings and Branch Conditions

| Stage | What happens |
|---|---|
| Input | Can be any len (including 0) |
| Padding | If len > 56 after adding flag, requires extra block transform |
| Output | Always 32 bytes in big-endian |

## Dependencies

- `<stdint.h>`, `<string.h>` (memcpy, memset)
- Used from `crypto_wallet.c` when `USE_CRYPTO_SIGN=0`

## Relations

- `crypto_wallet.md` (fallback provider)
- `task_sign.md` (consumer in minimal build)
