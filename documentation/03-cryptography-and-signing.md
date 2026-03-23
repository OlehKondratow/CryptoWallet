# 3. Cryptography and signing

## 3.1 Goals

- Derive keys from a BIP-39 mnemonic (when signing enabled), use BIP-32 for account paths.
- Sign Bitcoin-style transactions with **ECDSA secp256k1** (trezor-crypto stack).
- Use STM32 hardware TRNG where the stack expects random bytes; fail closed where implemented.

## 3.2 Build modes

| `USE_CRYPTO_SIGN` | Behaviour |
|---------------------|-----------|
| `1` | trezor-crypto: RNG, BIP-39/32, ECDSA, related hashes |
| `0` | Reduced path (e.g. SHA-256 minimal) — **not** a hardware wallet product configuration |

`USE_TEST_SEED=1` forces known test mnemonic — **only** for lab/CI; keys are predictable.

## 3.2b Private key derivation (how keys are produced)

- **Seed in:** `get_wallet_seed()` fills a **64-byte BIP-39 seed** (or returns an error). There is **no** separate “generate private key from RNG only” path in the signing task—the seed is the root secret.
- **Derivation:** `crypto_derive_btc_m44_0_0_0_0()` in `Core/Src/crypto_wallet.c` calls trezor-crypto: `hdnode_from_seed()` then **BIP-32** path **`m/44'/0'/0'/0/0`** (Bitcoin legacy default used by this project).
- **Deterministic:** for a **fixed** seed, the secp256k1 private key at that path is **fully determined**—derivation does **not** consume TRNG. Changing the path or seed changes the key.
- **Where TRNG still matters:** ECDSA **signing** uses randomness (and library internals may call the RNG); **creating a new mnemonic** from entropy uses `crypto_entropy_to_mnemonic_12()` with **128 bits of entropy** (caller must supply entropy—e.g. from TRNG in a full wallet product). The **`task_sign`** path only **loads** seed and derives— it does not implement “create new wallet” end-to-end.

## 3.3 Signing pipeline (conceptual)

1. Ingress (HTTP `POST /tx` or WebUSB) parses recipient/amount/currency.
2. `tx_request_validate()` enforces format and policy gates.
3. Request is queued for `task_sign`; user confirms on button.
4. `crypto_wallet` derives keys and signs; response exposed via `GET /tx/signed` or WebUSB completion path.

**HTTP caveat:** handler may return **200** with HTML even when validation fails—clients must poll signed status or trust OLED outcome.

## 3.4 Memory hygiene

- **`memzero()`** — volatile stores to clear sensitive buffers; compiler must not remove (see implementation).
- **Stacks** — fixed sizes in FreeRTOS configuration; no unbounded alloc on signing hot path by design.

## 3.4b Seed / private-key storage (honest scope)

- **Provisioning hook:** `get_wallet_seed()` (see `task_sign.c`). The default **weak** implementation returns **failure**—there is **no** built-in “secure vault” in this repo that persists user keys.
- **`USE_TEST_SEED=1`:** `wallet_seed.c` injects a **known** test mnemonic for lab/CI only; **not** a production secure store.
- **Production intent** (from source comments): replace with **secure element**, **encrypted flash**, or another **threat-modelled** design—outside the current minimal integration.
- **Distinction:** bootloader **image** signing keys (`stm32_secure_boot`) and **wallet** keys (BIP-32/39) are **different** secrets and trust domains.

## 3.5 TRNG and device-bound seed storage

This section ties together **hardware randomness**, **lab verification**, and a **future** design for **encrypted seed at rest** bound to the board. None of the storage binding is implemented yet; TRNG **is** used by the crypto stack where enabled.

### 3.5.1 TRNG in firmware (today)

- **Hardware RNG** (`HAL_RNG` / STM32 TRNG) feeds **trezor-crypto** and related code when signing and crypto features are enabled; pooling and API usage follow library expectations.
- **Role:** entropy for runtime crypto (nonces, key generation paths inside the library, etc.)—**not** the same as “UART dump mode” below.

### 3.5.2 UART dump mode (lab / CI only)

- **`USE_RNG_DUMP=1`:** a dedicated task streams **raw** TRNG bytes to UART for **external statistical tests** (e.g. capture script + dieharder). **CWUP** does not run on that UART in this mode.
- **Do not** treat the UART stream as the production path for provisioning **stored** keys: production code should call the **in-firmware** RNG driver directly. Operational detail and CI semantics: [`06-integrity-rng-verification.md`](06-integrity-rng-verification.md).

### 3.5.3 TRNG vs UID in a future encrypted vault

| Input | Typical role |
|-------|----------------|
| **TRNG (hardware)** | **Entropy** for one-time values: per-install **salt** written next to ciphertext, **nonces** for AEAD, any random material that must not be predictable or copied from another unit. |
| **UID (96-bit, factory)** | **Binding**: stable per-chip value mixed into **KDF** so ciphertext + PIN from **this** flash does not decrypt on **another** MCU. **Not** a secret—see limits below. |

A sound design uses **both**: randomness from **TRNG** for cryptographic freshness; **UID** (and PIN) to stop **portable** flash clones. Do **not** use UID alone as the sole “entropy” for wrapping the seed.

### 3.5.4 Device-bound seed on disk (design idea — not implemented)

**Intent:** persist an **encrypted** seed (or wrapped seed) in **on-chip or external flash**, but **bind** decryption to **this** board so the same ciphertext **cannot** be unlocked on another device—even if someone copies the flash image and knows the user PIN.

**Sketch:**

1. **Unique device input:** read **UID** (e.g. STM32 `UID` registers), plus a **random salt** generated once at first setup (**from TRNG**, §3.5.3) and stored next to the ciphertext.
2. **Key derivation:** `K = KDF(PIN || hardware_salt || UID || stored_salt)` (algorithm and iteration count are a separate design choice).
3. **At rest:** store only **AEAD ciphertext + metadata**—**never** the raw seed.
4. **Unlock:** user enters PIN; firmware recomputes `K` with **local UID**; decrypt fails on another chip.

**What this helps with:** cloned flash on another board; casual restore of a raw dump without an explicit export flow.

**What it does *not* solve:** UID is not a hidden root-of-trust; physical theft of **this** device + PIN still decrypts; user portability to a new device needs an **explicit** backup/export path.

**Relation to other options:** compatible with **secure element**, **encrypted flash**, or Trezor-like **PIN + encrypted DEK**—board binding is an **additional** KDF input.

## 3.6 What this document does **not** cover

- Bootloader signing algorithms and key storage in `stm32_secure_boot` — see that repository.
- Formal side-channel analysis — not claimed here.
