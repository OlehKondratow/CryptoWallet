# 🧪 Test Scripts for RNG Security & Transaction Signing

**Repository:** CryptoWallet  
**Date Created:** 2026-03-19  
**Status:** ✅ Ready for Production Testing  

---

## 📋 Overview

This directory contains comprehensive testing scripts for validating:

1. **RNG Security** — Random Number Generator quality and statistical properties
2. **Transaction Signing** — Bitcoin-style ECDSA signing with RFC6979 determinism
3. **Security Infrastructure** — memzero, key derivation, certificate validation

---

## 📁 Files

### Main Testing Scripts

#### 1. **test_rng_signing_comprehensive.py** (23 KB, 550+ lines)

Comprehensive Python testing framework with 4 modes:

- **`--mode rng`** — Capture and analyze RNG data from UART
- **`--mode dieharder`** — Run DIEHARDER statistical tests  
- **`--mode signing`** — Test transaction signing over HTTP
- **`--mode verify-all`** — Automated full security verification

```bash
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

#### 2. **test_commands_reference.sh** (11 KB, 250+ lines)

Ready-to-use Bash command reference with 9 testing phases:

1. Install dependencies
2. Build firmware for RNG
3. Flash device
4. Capture RNG data (30 min)
5. Analyze RNG quality
6. Run DIEHARDER tests (1-3 hours)
7. Rebuild firmware for signing
8. Test transaction signing
9. Generate final report

```bash
bash scripts/test_commands_reference.sh
```

---

## 🚀 Quick Start

### Minimal Setup (5 minutes)

```bash
# Install dependencies
pip install pyserial requests
sudo apt install dieharder

# Run help
python3 scripts/test_rng_signing_comprehensive.py --help
```

### Full RNG Testing (4+ hours)

```bash
# Phase 1: Build & Flash (5 min)
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash

# Phase 2: Capture RNG (30 min)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Phase 3: DIEHARDER (1-3 hours)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Phase 4: Analyze
ls -lh rng.bin dieharder_results.txt
```

### Quick Signing Test (10 minutes)

```bash
# Rebuild with HTTP
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4
make flash

# Test signing
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
```

---

## 📊 Testing Modes

### RNG Mode (`--mode rng`)

**Purpose:** Capture and analyze raw entropy from STM32H743 TRNG + LCG

**What it does:**
1. Checks dependencies (pyserial, dieharder)
2. Verifies UART port availability
3. Captures 128 MiB binary data (@ 115200 baud)
4. Performs quick analysis:
   - Shannon entropy (max 8.0 bits/byte)
   - Byte distribution statistics
   - Chi-square test

**Requirements:**
- Firmware built with `USE_RNG_DUMP=1`
- UART connected to `/dev/ttyACM0` (or custom `--port`)
- 30 minutes for 128 MiB @ 115200 baud

**Output:**
- `rng.bin` — 128 MiB binary file
- Console entropy analysis

**Example:**
```bash
python3 scripts/test_rng_signing_comprehensive.py --mode rng \
  --port /dev/ttyACM0 \
  --bytes 134217728 \
  --output rng.bin
```

---

### DIEHARDER Mode (`--mode dieharder`)

**Purpose:** Run ~100+ statistical tests on RNG data

**What it does:**
1. Verifies file size (minimum 1 MiB)
2. Runs DIEHARDER with `dieharder -g 201 -f <file>`
3. Reports PASS/FAIL/WEAK for each test

**Requirements:**
- `dieharder` package (`sudo apt install dieharder`)
- Binary RNG file (rng.bin)
- 1-3 hours for complete test suite

**Interpretation:**
- **0.001-0.999 p-value** → PASS ✅
- **0.0001-0.001 p-value** → WEAK ⚠️
- **< 0.0001 p-value** → FAIL ❌

**Example:**
```bash
# All tests
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Specific test
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --test 1

# With custom file
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng_256mb.bin
```

---

### Signing Mode (`--mode signing`)

**Purpose:** Test transaction signing and RFC6979 determinism

**What it does:**
1. Verifies device connectivity (HTTP ping)
2. Tests signing with various inputs:
   - Amounts: 0.1, 0.5, 1.0, 10.5, 0.00001
   - Addresses: 3 Bitcoin test addresses
   - Currencies: BTC, ETH, LTC
3. Verifies RFC6979: same TX → same signature (3 attempts)

**Requirements:**
- Firmware built with `USE_LWIP=1`
- Device on network (DHCP or static 192.168.0.10)
- HTTP endpoint accessible
- `requests` Python package

**Output:**
- Sign success/failure for each test combination
- RFC6979 determinism verification

**Example:**
```bash
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
```

---

### Verify-All Mode (`--mode verify-all`)

**Purpose:** Automated complete security testing

**Phases:**
1. RNG analysis
2. RNG data capture (128 MiB)
3. DIEHARDER statistical tests
4. Transaction signing tests
5. RFC6979 determinism check

**Duration:** 3-5 hours

**Example:**
```bash
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

---

## 📖 Documentation

- **Full Guide:** [`TESTING_GUIDE_RNG_SIGNING.md`](./TESTING_GUIDE_RNG_SIGNING.md)
- **Crypto Details:** [`crypto/`](./crypto/)
- **Test Plan:** [`test_plan_signing_rng.py`](../scripts/test_plan_signing_rng.py)

---

## ✅ Requirements

### Python Packages
```bash
pip install pyserial requests
```

### System Packages
```bash
sudo apt install dieharder doxygen
```

### Firmware Configuration

**For RNG:**
```bash
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1
```

**For Signing:**
```bash
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1
```

---

## 🔍 Examples

### Example 1: Full RNG Test (Morning)

```bash
# Build
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash

# Capture (30 min)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Quick analysis
file rng.bin
sha256sum rng.bin
```

### Example 2: DIEHARDER (Full Day)

```bash
# From morning capture
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder | tee dieharder.txt

# Analyze
grep -c "PASS" dieharder.txt
grep -c "FAIL" dieharder.txt
grep "FAIL" dieharder.txt | head -5
```

### Example 3: Signing Test (Afternoon)

```bash
# Rebuild
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4
make flash

# Test
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
```

### Example 4: Complete Verification

```bash
# All phases automated
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all 2>&1 | tee full_test.log

# Archive results
tar -czf cryptowallet_test_$(date +%Y%m%d).tar.gz rng.bin dieharder_results.txt full_test.log
```

---

## 📈 Interpreting Results

### RNG Entropy

| Value | Meaning | Action |
|-------|---------|--------|
| > 7.9 bits/byte | Excellent | ✅ OK |
| 7.5-7.9 bits/byte | Good | ✅ OK |
| 7.0-7.5 bits/byte | Acceptable | ⚠️ Monitor |
| < 7.0 bits/byte | Poor | ❌ Investigate |

### DIEHARDER Results

| p-value Range | Result | Action |
|---------------|--------|--------|
| 0.001-0.999 | PASS | ✅ OK |
| 0.0001-0.001 | WEAK | ⚠️ Retest (256 MiB) |
| < 0.0001 | FAIL | ❌ Investigate RNG |
| > 0.9999 | FAIL | ❌ Investigate RNG |

### Signing Tests

- ✅ All transactions signed → OK
- ✅ Identical signatures → RFC6979 working
- ❌ Some failed → Check button confirmation
- ❌ Different signatures → Check RFC6979

---

## 🛠️ Troubleshooting

### UART Capture Not Working

```bash
# Check port
ls -la /dev/ttyACM0

# Check firmware
screen /dev/ttyACM0 115200
# Should see binary data, no text

# Try different port
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyUSB0
```

### Low Entropy Detected

```bash
# Verify TRNG in code
grep "HAL_RNG" Core/Src/crypto_wallet.c

# Check LCG parameters
grep "1664525\|1013904223" Core/Src/crypto_wallet.c

# Rebuild
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash
```

### Signing Test Fails

```bash
# Check connectivity
ping 192.168.0.10
curl http://192.168.0.10/status

# Check firmware
grep "USE_LWIP" build/firmware.elf

# Verify compilation
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4
make flash
```

---

## 🔗 Integration with CI/CD

### GitHub Actions Example

```yaml
name: Security Tests
on: [push, pull_request]

jobs:
  rng-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: pip install pyserial requests
      - run: sudo apt install dieharder
      - run: python3 scripts/test_rng_signing_comprehensive.py --mode rng
      - uses: actions/upload-artifact@v2
        with:
          name: rng-results
          path: rng.bin
```

---

## 📞 Support

```bash
# Help
python3 scripts/test_rng_signing_comprehensive.py --help

# Examples
python3 scripts/test_rng_signing_comprehensive.py --help | grep -A 10 "Examples:"
```

---

**Status:** ✅ Production Ready  
**Version:** 1.0  
**Last Updated:** 2026-03-19
