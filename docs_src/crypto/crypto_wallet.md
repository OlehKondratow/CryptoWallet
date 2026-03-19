\page crypto_wallet "crypto_wallet: trezor-crypto glue (RNG/BIP32/ECDSA)"
\related crypto_hash_sha256
\related crypto_derive_btc_m44_0_0_0_0
\related crypto_sign_btc_hash
\related crypto_entropy_to_mnemonic_12
\related random_buffer

# `crypto_wallet.c` + `crypto_wallet.h`

<brief>The `crypto_wallet` module wraps the trezor-crypto library: it brings up STM32 RNG with entropy mixing, provides BIP-39 mnemonics, BIP-32 HDNode derivation, and ECDSA secp256k1 signing for the standard Bitcoin path m/44'/0'/0'/0/0.</brief>

## Overview

The `crypto_wallet` module is an adaptation layer between the project and trezor-crypto: on one side, RNG is used (STM32 TRNG + software entropy pool for stability), on the other side, "simple" APIs are exported for higher levels (sign_hash, derive_key, mnemonic_from_entropy). When `USE_CRYPTO_SIGN=0`, all functions become stubs, and instead of trezor, `sha256_minimal` is used for hashing. Business role: provide one control point for all cryptography in the project.

## Logic Flow (RNG + Crypto Pipeline)

### RNG Initialization

1. `crypto_rng_init()` is called once from `main.c`.
2. Takes the current `HAL_GetTick()` and applies LFSR transformation for nonlinear mix function.
3. Result is XORed into global `s_rng_entropy_pool`.
4. Then each call to `random_buffer()` (from trezor-crypto):
   - tries to get a word from STM32 TRNG (if `hrng.Instance` is valid)
   - XORs with `s_rng_entropy_pool` and updates pool via LCG (multiplicative congruential generator)
   - copies result to output buffer

### Cryptographic Paths

**When `USE_CRYPTO_SIGN=1`:**
- BIP-39: mnemonic_from_data (16 bytes → 12 words)
- BIP-32: hdnode_from_seed, then chain of derive operations for m/44'/0'/0'/0/0
- ECDSA: ecdsa_sign_digest with secp256k1, compact format r||s (64 bytes)

**When `USE_CRYPTO_SIGN=0`:**
- All functions return -1 (not supported)
- SHA-256 falls back to `sha256_minimal`

## Interrupts and Registers

Module uses HAL RNG and `HAL_GetTick()` for entropy, but does not touch direct ISR/registers. The only detail — in `crypto_sign_btc_hash()` after signing, `memzero()` is called to clear the private key.

## Timings and Branching Conditions

| Function | When It Works | Error |
|----------|---------------|-------|
| `crypto_rng_init` | once at startup (optional when USE_CRYPTO_SIGN=0) | -1 if hrng.Instance NULL |
| `random_buffer` | constantly on trezor requests | no-op if NULL buf/len |
| `crypto_entropy_to_mnemonic_12` | on setup; reads 16 bytes entropy | -1 if USE_CRYPTO_SIGN=0 or bad buffer |
| `crypto_derive_btc_m44_0_0_0_0` | on signing; takes seed (64 bytes) | -1 on derivation error or USE_CRYPTO_SIGN=0 |
| `crypto_sign_btc_hash` | on signing; takes key+digest | -1 on sign error or USE_CRYPTO_SIGN=0; clears key after |
| `crypto_hash_sha256` | when forming input for signature | -1 if NULL input/output |

## Dependencies

Direct dependencies:
- **trezor-crypto:** `bip39.h`, `bip32.h`, `ecdsa.h`, `secp256k1.h`, `rand.h`, `sha2.h` (when `USE_CRYPTO_SIGN`)
- **sha256_minimal.h** (fallback when `USE_CRYPTO_SIGN=0`)
- **memzero.h** to clear sensitive buffers
- **HAL RNG:** `stm32h7xx_hal.h` and `hrng` initialization from `hw_init.c`

Globals:
- `s_rng_entropy_pool` (local static pool)
- `hrng` (from `hw_init` via weak reference)
- `random_buffer()` (hook that trezor-crypto expects)

## Module Relationships

- `task_sign.md` (consumer seed derivation + signing)
- `memzero.md` (key clearing)
- `sha256_minimal.md` (fallback hashing)
- `wallet_seed.md` (seed management)
- `hw_init.md` (RNG initialization)
