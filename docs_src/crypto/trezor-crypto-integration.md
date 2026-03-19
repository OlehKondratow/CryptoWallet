\page trezor-crypto-integration "trezor-crypto: Integration and cryptographic primitives"

# trezor-crypto Integration

<brief>CryptoWallet integrates trezor-crypto for Bitcoin cryptographic operations: BIP-39 mnemonic generation, BIP-32 HD key derivation, ECDSA secp256k1 signing with RFC6979, SHA-256 hashing, and entropy management via STM32 TRNG + entropy pool mixing.</brief>

## Overview

The CryptoWallet firmware delegates all cryptographic operations to **trezor-crypto**, a well-tested, production-grade cryptocurrency library (MIT License) used in Trezor hardware wallets. Integration is conditional on the `USE_CRYPTO_SIGN=1` compile flag; without it, only SHA-256 remains available via `sha256_minimal.c`.

## Cryptographic Components

### 1. BIP-39 Mnemonic Generation

**Module:** `bip39.c`

**Purpose:** Convert 128-bit random entropy into a 12-word mnemonic phrase using the BIP-39 English wordlist.

**Implementation in CryptoWallet:**
```c
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[16],
                                  char *mnemonic_out, size_t mnemonic_size);
```

**Flow:**
1. Generate 128-bit entropy from `random_buffer()` (STM32 TRNG + pool)
2. Call trezor's `mnemonic_from_data(entropy, 16, mnemonic_out)`
3. Output: space-separated 12 words (e.g., "abandon wallet ... about")

**Example Test Vector:**
- Entropy: all zeros (not recommended in production)
- Mnemonic: "abandon abandon abandon ... abandon about"
- Bitcoin Address: `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`

### 2. BIP-39 Mnemonic → Seed

**Modules:** `pbkdf2.c`, `hmac.c`

**Purpose:** Convert mnemonic + optional passphrase into a 64-byte seed for BIP-32 derivation.

**Implementation:**
```c
// Wrapped in trezor; CryptoWallet calls:
int mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                     uint8_t seed_out[64], size_t max_len);
```

**PBKDF2 Parameters (BIP-39 standard):**
- HMAC-SHA512
- Iterations: 2048
- Salt: "TREZOR" prefix + passphrase

**Output:** 64 bytes used as root seed for BIP-32.

### 3. BIP-32 Hierarchical Deterministic Keys

**Modules:** `bip32.c`, `curves.c`, `secp256k1.c`

**Purpose:** Derive child keys from master seed along a specific derivation path.

**Standard Path in CryptoWallet:** `m/44'/0'/0'/0/0`

- `44` — BIP-44 (multi-purpose HD wallets)
- `0'` — Bitcoin coin (hardened)
- `0'` — Account 0 (hardened)
- `0` — External chain
- `0` — First address

**Implementation in CryptoWallet:**
```c
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32]);
```

**Derivation Process:**
1. `hdnode_from_seed()` — Initialize root HDNode from 64-byte seed
2. `hdnode_private_ckd()` — Derive at each hardened/non-hardened step
3. `hdnode_fill_public_key()` — Compute public key (optional, for verification)
4. Output: 32-byte private key (big-endian scalar)

**Security:** Hardened derivation (`'` notation) ensures child keys cannot be derived from the public key alone.

### 4. ECDSA Signing (secp256k1)

**Modules:** `ecdsa.c`, `secp256k1.c`, `bignum.c`, `rfc6979.c`

**Purpose:** Sign a message digest using ECDSA with the secp256k1 elliptic curve (Bitcoin standard).

**Implementation in CryptoWallet:**
```c
int crypto_sign_btc_hash(uint8_t priv_key[32],
                        const uint8_t hash[32],
                        uint8_t sig_out[64]);
```

**Signing Algorithm:**
- **Curve:** secp256k1 (p = 2^256 - 2^32 - 977)
- **Hash Algorithm:** SHA-256
- **RFC6979:** Deterministic k generation (no RNG dependency)
- **Output:** 64-byte compact signature (r || s, both 32-byte big-endian)

**Deterministic k (RFC6979 Benefits):**
- Same private key + same message = identical signature
- Prevents RNG failure attacks
- Allows signature verification in tests

### 5. SHA-256 Hashing

**Modules:** `sha2.c`, `hasher.c`

**Purpose:** Hash arbitrary data using SHA-256 (FIPS 180-4).

**Implementation in CryptoWallet:**
```c
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[32]);
```

**Input:** Canonical transaction string formatted as `"recipient|amount|currency"`

**Example:**
- Input: `"1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA|0.5|BTC"`
- Output: 32-byte SHA-256 digest

**Fallback:** When `USE_CRYPTO_SIGN=0`, delegates to `sha256_minimal.c`.

### 6. Address Validation (Base58Check)

**Modules:** `base58.c`, `address.c`

**Purpose:** Validate Bitcoin addresses using Base58Check encoding (checksum verification).

**Usage in CryptoWallet:** `tx_request_validate.c` calls trezor's `base58_decode_check()`.

**Supported Format:** Legacy P2PKH addresses (start with '1', 25 bytes total).

### 7. Entropy and Random Number Generation

**Modules:** `rand.c`, `hmac_drbg.c`

**STM32H743 TRNG Integration:**
```c
int crypto_rng_init(void);  // Called once from main.c
void random_buffer(uint8_t *buf, size_t len);  // Called by trezor-crypto
```

**Entropy Sources:**
1. **Hardware TRNG:** `HAL_RNG_GenerateRandomNumber()` → 32-bit words
2. **Entropy Pool:** Software LCG mixing (XOR + state update)
3. **Combination:** `rng_val = HAL_RNG_value XOR entropy_pool`

**LCG Parameters (Numerical Recipes):**
- Multiplier: `1664525`
- Increment: `1013904223`
- State update: `pool = (pool * 1664525) + 1013904223`

**Quality:** Combines hardware randomness with software whitening to defend against TRNG bias.

## Complete Signing Pipeline

```
Network Input (HTTP POST /tx or WebUSB)
    ↓
[tx_request_validate.c]
    ├─ Base58Check decode: recipient address
    ├─ Regex: amount format (decimal)
    ├─ Whitelist: currency ("BTC", "ETH", ...)
    ↓
[task_sign.c → crypto_wallet.c]
    ├─ build_hash_input(): Format "recipient|amount|currency"
    ├─ crypto_hash_sha256(): 32-byte digest
    ├─ User confirmation: g_user_event_group (button press)
    ├─ get_wallet_seed(): 64-byte BIP-39 seed
    ├─ crypto_derive_btc_m44_0_0_0_0(): 32-byte private key
    ├─ crypto_sign_btc_hash(): 64-byte signature (r||s)
    ├─ memzero(): Clear sensitive buffers
    ↓
[Output]
    ├─ USB: WebUSB_NotifySignatureReady(64-byte sig)
    ├─ Network: HTTP JSON response with hex-encoded signature
    ↓
Complete
```

## Compilation Options

### `USE_CRYPTO_SIGN=1` (Default for production)

**Linked Modules (22 total):**
- **BIP:** bip39, bip32
- **ECDSA:** ecdsa, secp256k1, curves, bignum, rfc6979
- **Hash:** sha2, hasher, sha3, blake256, blake2b, blake2s, groestl
- **HMAC:** hmac, hmac_drbg, pbkdf2
- **Encoding:** base58, address, ripemd160
- **Other:** rand, nist256p1
- **ED25519:** (linked but not used in current CryptoWallet)

**Binary Size:** ~100-150 KB object code

**Features:**
- Full ECDSA signing
- BIP-39/BIP-32 derivation
- STM32 TRNG + entropy pool
- RFC6979 deterministic k

### `USE_CRYPTO_SIGN=0` (Minimal build)

**Linked Modules:**
- `sha2.c` (SHA-256 only from trezor-crypto)
- `sha256_minimal.c` (fallback, ~16 KB)

**Binary Size:** ~5 KB

**Features:**
- SHA-256 hashing only
- No ECDSA, BIP-39, BIP-32
- No key derivation

**Use Case:** Testing, UI validation, minimal embedded systems.

### `USE_TEST_SEED=1` (Development only)

**Behavior:**
- `get_wallet_seed()` returns hardcoded BIP-39 test vector
- Mnemonic: "abandon abandon ... about"
- Derived Bitcoin address: `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`
- Enables reproducible signatures for automated tests

**⚠️ WARNING:** Never use in production or with real funds.

## Security Considerations

### 1. Entropy Quality

**Multi-Source Mixing:**
- STM32H743 hardware TRNG provides physical randomness
- Software entropy pool adds temporal variance
- Combined via XOR to defend against single-source failure

**Validation:**
- Scripts: `scripts/capture_rng_uart.py`, `scripts/run_dieharder.py`
- NIST FIPS 140-2 statistical tests recommended

### 2. Key Material Handling

**Zeroization:**
- All sensitive buffers (seed, private key, digest) cleared via `memzero()`
- Uses volatile writes to prevent compiler optimization
- Keys never logged, displayed, or transmitted unencrypted

**Buffer Lifetime:**
- Private keys allocated on stack (local scope)
- Cleared before function return
- No global key storage in firmware

### 3. Signature Determinism

**RFC6979 Benefit:**
- Removes RNG dependency from ECDSA `k` generation
- Prevents accidental key reuse (same hash → same signature)
- Supports deterministic test vectors

**Trade-off:** Signature is deterministic, not random (standard in Bitcoin).

### 4. Mnemonic Security

**Protection:**
- BIP-39 seed computed on-demand, never persisted
- Immediately zeroed after key derivation
- No mnemonic wordlist stored in firmware flash

**Production Implementation:**
- Deploy secure element (SE) or encrypted flash for seed storage
- Use KDF with additional user passphrase
- Consider hardware wallet integration

## Dependencies and Attribution

**Library:** [trezor-crypto](https://github.com/trezor/trezor-crypto)
- **License:** MIT
- **Version:** Integrated as subproject in `ThirdParty/trezor-crypto/`
- **Maintainers:** SatoshiLabs (Trezor project)

**Related Documentation:**
- BIP-32: [Bitcoin Improvement Proposal 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)
- BIP-39: [Bitcoin Improvement Proposal 39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki)
- BIP-44: [Bitcoin Improvement Proposal 44](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki)
- RFC6979: [Deterministic ECDSA](https://tools.ietf.org/html/rfc6979)
- FIPS 180-4: SHA cryptographic hash functions

## Related Modules in CryptoWallet

- `crypto_wallet.md` — Wrapper API and RNG initialization
- `task_sign.md` — Signing FSM that calls crypto functions
- `memzero.md` — Secure buffer zeroing
- `wallet_seed.md` — Test seed stub (development only)
- `tx_request_validate.md` — Address validation and input sanitization
