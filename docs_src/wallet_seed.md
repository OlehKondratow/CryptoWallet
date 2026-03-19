\page wallet_seed "wallet_seed: test seed (development only)"
\related get_wallet_seed

# `wallet_seed.c`

<brief>The `wallet_seed` module is a stub for `get_wallet_seed()` when `USE_TEST_SEED=1`: returns a BIP-39 seed from a known test mnemonic "abandon...about", **never for real funds**.</brief>

## Overview

<brief>The `wallet_seed` module is a stub for `get_wallet_seed()` when `USE_TEST_SEED=1`: returns a BIP-39 seed from a known test mnemonic "abandon...about", **never for real funds**.</brief>

## Abstract (Logic Synthesis)

For learning and testing, a reproducible seed is needed. `wallet_seed` contains a hardcoded standard BIP-39 test vector (address `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`), allowing signature reproduction and signing-path verification. This is **development-only** — production must replace it with a secure element or encrypted flash storage. Business role: provide deterministic seed during development.

## Logic Flow

1. When `USE_CRYPTO_SIGN=1` and `USE_TEST_SEED=1`, the real implementation is compiled.
2. Function `get_wallet_seed()` simply calls `mnemonic_to_seed()` from trezor-crypto with the known string.
3. Copies the resulting 64 bytes into the output buffer and clears the local seed via `memzero()`.
4. Under any other flag — an empty stub is compiled, and `get_wallet_seed()` (weak symbol in `task_sign.c`) simply returns -1.

## Interrupts/Registers

No ISRs or registers: just string transformation and memcpy.

## Timings and Branch Conditions

| Build | Behavior |
|---|---|
| `USE_CRYPTO_SIGN=1` and `USE_TEST_SEED=1` | Returns seed from "abandon...about" |
| Any other | Weak stub: -1 (not implemented) |

## Dependencies

- `bip39.h` from trezor-crypto (`mnemonic_to_seed`)
- `memzero.h` for clearing local buffer
- Used from `task_sign.c` during signing

## Relations

- `task_sign.md` (consumer in signing pipeline)
- `crypto_wallet.md` (wrapper over trezor)
- `memzero.md` (seed cleanup)
