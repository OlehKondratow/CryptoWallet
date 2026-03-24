#!/bin/bash
# CryptoWallet RNG & Signing Tests - Quick Command Reference
# All required commands for a full testing cycle

# ============================================================================
# DEPENDENCY INSTALLATION
# ============================================================================

echo "=== Installing Dependencies ==="

# Python packages
pip install pyserial requests

# System packages
sudo apt update
sudo apt install doxygen dieharder

# ============================================================================
# PHASE 1: RNG TEST PREPARATION
# ============================================================================

echo ""
echo "=== Phase 1: RNG Preparation ==="

cd /data/projects/CryptoWallet

# Build firmware with RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4

# Size check
arm-none-eabi-size build/firmware.elf

# Flash firmware to device
make flash

# Verify trezor-crypto integration
arm-none-eabi-nm build/firmware.elf | grep -E "bip39|ecdsa|secp256k1"

# ============================================================================
# PHASE 2: RNG DATA CAPTURE (30 min)
# ============================================================================

echo ""
echo "=== Phase 2: RNG Data Capture (30 minutes at 115200 baud) ==="

# Check UART output (should be raw bytes only)
# screen /dev/ttyACM0 115200
# (Ctrl-A, Ctrl-\ to exit)

# Capture 128 MiB of RNG data (standard size for Dieharder)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0

# File check
ls -lh rng.bin
file rng.bin

# If you need 256 MiB (for a more robust test)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --bytes 268435456 --output rng_256mb.bin

# ============================================================================
# PHASE 3: RNG QUALITY ANALYSIS
# ============================================================================

echo ""
echo "=== Phase 3: RNG Quality Analysis ==="

# Quick analysis (entropy, chi-square, distribution)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --file rng.bin

# File statistics
stat rng.bin

# SHA256 for integrity verification
sha256sum rng.bin > rng.bin.sha256

# ============================================================================
# PHASE 4: DIEHARDER STATISTICAL TESTS (1-3 hours)
# ============================================================================

echo ""
echo "=== Phase 4: DIEHARDER Statistical Tests (1-3 hours) ==="

# List available tests
dieharder -l

# Run all tests (typically 100+)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin \
  | tee dieharder_results.txt

# Or specific tests for debugging
dieharder -g 201 -f rng.bin -d 1          # Test 1 (birthdays)
dieharder -g 201 -f rng.bin -d 2          # Test 2
dieharder -g 201 -f rng.bin -d 1 -p 0     # Test 1, subtest 0

# Analyze results
grep "PASS" dieharder_results.txt | wc -l
grep "FAIL" dieharder_results.txt | wc -l
grep "WEAK" dieharder_results.txt | wc -l

# Save results
cp dieharder_results.txt dieharder_results_$(date +%Y%m%d_%H%M%S).txt

# ============================================================================
# PHASE 5: SIGNING TEST PREPARATION
# ============================================================================

echo ""
echo "=== Phase 5: Transaction Signing Preparation ==="

# Rebuild firmware with HTTP instead of RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4

# Flash firmware
make flash

# Check configuration
grep -n "USE_LWIP\|USE_CRYPTO_SIGN\|USE_TEST_SEED" Makefile | head -10

# ============================================================================
# PHASE 6: DEVICE CHECK
# ============================================================================

echo ""
echo "=== Phase 6: Device Connectivity Check ==="

# Wait for DHCP to come up (if used)
sleep 5

# Ping device
ping -c 3 192.168.0.10

# Check status via curl
curl http://192.168.0.10/status
# Expected response: {"status":"ready"}

# Or via Python requests
python3 << 'EOF'
import requests
try:
    r = requests.get('http://192.168.0.10/status', timeout=5)
    print(f"Status: {r.status_code}")
    print(f"Response: {r.json()}")
except Exception as e:
    print(f"Error: {e}")
EOF

# ============================================================================
# PHASE 7: TRANSACTION SIGNING TESTS
# ============================================================================

echo ""
echo "=== Phase 7: Transaction Signing Tests ==="

# Run full signing test suite
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Or with a different IP
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.1.100

# Manual transaction submission (for debugging)
python3 << 'EOF'
import requests
import json

tx = {
    "recipient": "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA",
    "amount": "0.5",
    "currency": "BTC"
}

# POST transaction
r = requests.post("http://192.168.0.10/tx", json=tx, timeout=5)
print(f"POST: {r.status_code}")

# Wait for confirmation (press the device button!)
import time
time.sleep(2)

# GET signature
r = requests.get("http://192.168.0.10/tx/signed", timeout=5)
print(f"GET: {r.status_code}")
print(f"Response: {r.json()}")
EOF

# ============================================================================
# PHASE 8: DETERMINISM CHECK (RFC6979)
# ============================================================================

echo ""
echo "=== Phase 8: RFC6979 Determinism Test ==="

# Verify: identical TX -> identical signatures
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10 \
  | grep -A 10 "Deterministic"

# ============================================================================
# PHASE 9: COMPLETE TESTING (ALL SYSTEMS)
# ============================================================================

echo ""
echo "=== Phase 9: Complete Verification (ALL) ==="

# Automatically runs: RNG capture + DIEHARDER + signing tests
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all \
  | tee full_test_results_$(date +%Y%m%d_%H%M%S).txt

# ============================================================================
# ADDITIONAL DEBUG COMMANDS
# ============================================================================

echo ""
echo "=== Debug Commands ==="

# 1. Check presence of Doxygen comments
rg "@brief|@details" Core/Src/*.c | head -20

# 2. Regenerate documentation
make docs-md
make docs-doxygen
make docs

# 3. Check trezor-crypto configuration
arm-none-eabi-nm build/firmware.elf | grep random_buffer
arm-none-eabi-nm build/firmware.elf | grep memzero

# 4. Check section sizes
arm-none-eabi-objdump -h build/firmware.elf | grep -E "\.text|\.rodata|\.data"

# 5. List all symbols
arm-none-eabi-nm build/firmware.elf | sort | tail -50

# 6. Check LCG parameters
grep -n "1664525\|1013904223" Core/Src/crypto_wallet.c

# 7. Check memzero usage
grep -n "memzero" Core/Src/crypto_wallet.c

# 8. Check RFC6979
grep -n "rfc6979\|is_canonical" Core/Src/crypto_wallet.c

# ============================================================================
# FINAL REPORT COMMANDS
# ============================================================================

echo ""
echo "=== Generate Test Report ==="

# Create final report
cat > cryptowallet_test_report.md << REPORT
# CryptoWallet Security Test Report

**Date:** $(date)
**Firmware:** $(arm-none-eabi-size build/firmware.elf | tail -1)

## RNG Analysis
- File: $(ls -lh rng.bin | awk '{print $9, $5}')
- SHA256: $(sha256sum rng.bin | cut -d' ' -f1)
- Entropy: [fill from results]
- Chi-square: [fill from results]

## DIEHARDER Results
- PASS tests: $(grep -c "PASS" dieharder_results.txt 2>/dev/null || echo "?")
- FAIL tests: $(grep -c "FAIL" dieharder_results.txt 2>/dev/null || echo "?")
- WEAK tests: $(grep -c "WEAK" dieharder_results.txt 2>/dev/null || echo "?")

## Signing Verification
- Device IP: 192.168.0.10
- HTTP Status: $(curl -s http://192.168.0.10/status | jq . 2>/dev/null || echo "Offline")
- Test Results: [fill manually]
- RFC6979 Determinism: [fill manually]

## Recommendations
- [fill manually]

REPORT

cat cryptowallet_test_report.md

# ============================================================================
# CLEANUP AND ARCHIVING
# ============================================================================

echo ""
echo "=== Cleanup and Archive ==="

# Archive results
mkdir -p test_results_$(date +%Y%m%d)
cp rng.bin dieharder_results.txt cryptowallet_test_report.md test_results_$(date +%Y%m%d)/

# Compress archive
tar -czf cryptowallet_tests_$(date +%Y%m%d).tar.gz test_results_$(date +%Y%m%d)/

# Print archive info
ls -lh cryptowallet_tests_*.tar.gz

# ============================================================================
# SUCCESSFUL COMPLETION
# ============================================================================

echo ""
echo "✓ All tests completed successfully!"
echo ""
echo "Results:"
echo "  - rng.bin (128 MiB binary RNG data)"
echo "  - dieharder_results.txt (statistical test output)"
echo "  - cryptowallet_test_report.md (summary report)"
echo "  - test_results_YYYYMMDD/ (archived results)"
echo ""
echo "Next steps:"
echo "  1. Review dieharder_results.txt for failures"
echo "  2. Verify RFC6979 determinism in test output"
echo "  3. Archive results for compliance documentation"
echo ""
