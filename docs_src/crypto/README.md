# 🔐 CryptoWallet Cryptography Documentation

## 📚 Core Documentation

### Cryptographic Adapter & Integration
- **[crypto_wallet.md](crypto_wallet.md)** - trezor-crypto adapter for CryptoWallet project
  - Available in: [Polish](crypto_wallet_pl.md), [Russian](crypto_wallet_ru.md)

### Security & Signing Tasks
- **[task_security.md](task_security.md)** - FSM for security tasks and key management
  - Available in: [Polish](task_security_pl.md), [Russian](task_security_ru.md)

### Cryptographic Utilities
- **[memzero.md](memzero.md)** - Secure memory clearing with `memzero()`
  - Available in: [Polish](memzero_pl.md), [Russian](memzero_ru.md)
- **[sha256_minimal.md](sha256_minimal.md)** - Minimal SHA-256 implementation
  - Available in: [Polish](sha256_minimal_pl.md), [Russian](sha256_minimal_ru.md)

## 🎲 Random Number Generation & Testing

### RNG Setup & Configuration
- **[rng_dump_setup.md](rng_dump_setup.md)** - Enable RNG dump from firmware for testing
  - Available in: [Polish](rng_dump_setup_pl.md), [Russian](rng_dump_setup_ru.md)

### Dieharder Testing Framework
- **[install_dieharder_setup.md](install_dieharder_setup.md)** - Dieharder installation and setup
  - Available in: [Polish](install_dieharder_setup_pl.md), [Russian](install_dieharder_setup_ru.md)

### Troubleshooting & Diagnostics
- **[rng_capture_troubleshooting.md](rng_capture_troubleshooting.md)** - RNG capture diagnostics and problem resolution
  - Available in: [Polish](rng_capture_troubleshooting_pl.md), [Russian](rng_capture_troubleshooting_ru.md)

### Testing Checklists
- **[rng_test_checklist.txt](rng_test_checklist.txt)** - Step-by-step RNG testing procedures
  - Available in: [Polish](rng_test_checklist_pl.txt), [Russian](rng_test_checklist_ru.txt)

## 📊 Technical Reference

### Analysis & Design Documents
- **[ANALYSIS_trezor_crypto_dieharder.md](ANALYSIS_trezor_crypto_dieharder.md)** - Complete technical analysis of trezor-crypto integration and RNG testing
- **[DIAGRAMS_trezor_crypto_dieharder.md](DIAGRAMS_trezor_crypto_dieharder.md)** - Architecture diagrams and data flow visualizations
- **[EXAMPLES_trezor_crypto_usage.md](EXAMPLES_trezor_crypto_usage.md)** - Complete working examples of cryptographic operations

## 🔐 Cryptographic Coverage

### BIP Standards
- **BIP-39:** Mnemonic seed generation
- **BIP-32:** Hierarchical deterministic key derivation

### Signing & Hashing
- **ECDSA:** Elliptic curve digital signature (secp256k1)
- **SHA-256:** Cryptographic hashing
- **RFC6979:** Deterministic ECDSA nonce generation

### Random Number Generation
- **STM32 TRNG:** Hardware true random number generator
- **LCG:** Linear congruential generator mixing
- **Dieharder:** Statistical randomness testing (~100+ tests)

### Security Features
- **memzero():** Secure buffer clearing
- **Key zeroization:** Prevent key recovery from memory
- **Address validation:** Format and checksum verification
- **Secure element integration:** Hardware-backed cryptography

## 🛠️ Quick Build & Test

### Build with crypto signing
```bash
make USE_CRYPTO_SIGN=1 -j4
make flash
```

### Capture and test RNG
```bash
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin
python3 scripts/run_dieharder.py --file rng.bin
```

---

**Last Updated:** 2026-03-20
