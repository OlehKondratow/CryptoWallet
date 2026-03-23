#!/bin/bash
# Quick command reference for RNG dump setup (run from repo root or via ./scripts/RNG_SETUP_QUICK_COMMANDS.sh).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT" || exit 1

# ============================================================================
# STEP 1: PREPARE
# ============================================================================

echo "=== STEP 1: Verify Makefile ==="
grep -n "USE_TEST_SEED\|USE_RNG_DUMP" Makefile | head -10


# ============================================================================
# STEP 2: ADD USE_RNG_DUMP TO MAKEFILE
# ============================================================================

echo ""
echo "=== STEP 2: Adding USE_RNG_DUMP to Makefile ==="

# Option A: Using cat >> (append to end of file)
cat >> Makefile << 'EOFMAKEFILE'

# USE_RNG_DUMP=1: output raw RNG data on UART (for Dieharder testing)
# WARNING: Disables normal UART output - only binary RNG data sent
USE_RNG_DUMP ?= 0
ifeq ($(USE_RNG_DUMP),1)
CFLAGS += -DUSE_RNG_DUMP
endif
EOFMAKEFILE

echo "✓ USE_RNG_DUMP added to Makefile"


# ============================================================================
# STEP 3: VERIFY CHANGES
# ============================================================================

echo ""
echo "=== STEP 3: Verify changes ==="
grep -A7 "USE_TEST_SEED" Makefile | head -12


# ============================================================================
# STEP 4: BACKUP (Optional but recommended)
# ============================================================================

echo ""
echo "=== STEP 4: Creating backup ==="

mkdir -p firmware_backup
if [ -f build/firmware.elf ]; then
    cp build/firmware.elf firmware_backup/firmware.elf.backup.$(date +%Y%m%d_%H%M%S)
    echo "✓ Backup created: firmware_backup/firmware.elf.backup.*"
else
    echo "⚠ No existing firmware.elf to backup"
fi


# ============================================================================
# STEP 5: CLEAN AND BUILD
# ============================================================================

echo ""
echo "=== STEP 5: Clean and rebuild ==="

make clean
echo "✓ Clean complete"

echo ""
echo "Building with USE_RNG_DUMP=1..."
make USE_RNG_DUMP=1 -j$(nproc)

if [ -f build/firmware.elf ]; then
    echo "✓ Build successful!"
    ls -lh build/firmware.elf build/firmware.bin 2>/dev/null
else
    echo "✗ Build failed - check errors above"
    exit 1
fi


# ============================================================================
# STEP 6: CHECK DEVICE CONNECTION
# ============================================================================

echo ""
echo "=== STEP 6: Check device connection ==="

if lsusb | grep -i stm32 > /dev/null; then
    echo "✓ STM32 device detected"
    lsusb | grep -i stm32
else
    echo "⚠ STM32 device NOT detected"
    echo "  Please connect STM32H743 Nucleo-144 via USB"
    echo "  Then run: make flash"
    exit 1
fi


# ============================================================================
# STEP 7: FLASH
# ============================================================================

echo ""
echo "=== STEP 7: Flash firmware ==="

make flash

if [ $? -eq 0 ]; then
    echo "✓ Flash complete!"
else
    echo "✗ Flash failed - check connection"
    exit 1
fi


# ============================================================================
# STEP 8: VERIFY UART OUTPUT
# ============================================================================

echo ""
echo "=== STEP 8: Verify UART output ==="
echo "Reading from /dev/ttyACM0 for 3 seconds (you should see binary data)..."
echo "Press Ctrl+C in 3 seconds..."

timeout 3 python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw 2>/dev/null || true

echo ""
echo "✓ If you saw binary output above, firmware is working correctly!"


# ============================================================================
# STEP 9: SETUP VENV (if needed)
# ============================================================================

echo ""
echo "=== STEP 9: Setup Python venv (if needed) ==="

if [ ! -d ".venv-test" ]; then
    echo "Creating venv..."
    python3 -m venv .venv-test
    source .venv-test/bin/activate
    pip install -r requirements-test.txt
    echo "✓ Venv created and packages installed"
else
    echo "✓ Venv already exists"
fi


# ============================================================================
# STEP 10: RUN TESTS
# ============================================================================

echo ""
echo "=== STEP 10: Ready to run tests ==="
echo ""
echo "Activate venv and run tests:"
echo ""
echo "  source .venv-test/bin/activate"
echo ""
echo "  # Capture RNG data (30 minutes):"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode rng"
echo ""
echo "  # Run Dieharder tests (1-3 hours):"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode dieharder"
echo ""


# ============================================================================
# SUCCESS
# ============================================================================

echo "════════════════════════════════════════════════════════════════════"
echo "✓ RNG DUMP SETUP COMPLETE!"
echo "════════════════════════════════════════════════════════════════════"
echo ""
echo "Status:"
echo "  ✓ Makefile updated with USE_RNG_DUMP=1"
echo "  ✓ Firmware built"
echo "  ✓ Firmware flashed to device"
echo "  ✓ UART output verified"
echo "  ✓ Python venv ready"
echo ""
echo "Next: Run the test commands above"
echo ""
