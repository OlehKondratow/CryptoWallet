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

## 3.3 Signing pipeline (conceptual)

1. Ingress (HTTP `POST /tx` or WebUSB) parses recipient/amount/currency.
2. `tx_request_validate()` enforces format and policy gates.
3. Request is queued for `task_sign`; user confirms on button.
4. `crypto_wallet` derives keys and signs; response exposed via `GET /tx/signed` or WebUSB completion path.

**HTTP caveat:** handler may return **200** with HTML even when validation fails—clients must poll signed status or trust OLED outcome.

## 3.4 Memory hygiene

- **`memzero()`** — volatile stores to clear sensitive buffers; compiler must not remove (see implementation).
- **Stacks** — fixed sizes in FreeRTOS configuration; no unbounded alloc on signing hot path by design.

## 3.5 RNG (firmware)

- Hardware RNG peripheral when enabled; software pooling/stack details follow trezor-crypto expectations.
- **`USE_RNG_DUMP=1`:** dedicated task streams raw bytes to UART for statistical testing—**not** mixed with CWUP text commands on that UART.

## 3.6 What this document does **not** cover

- Bootloader signing algorithms and key storage in `stm32_secure_boot` — see that repository.
- Formal side-channel analysis — not claimed here.
