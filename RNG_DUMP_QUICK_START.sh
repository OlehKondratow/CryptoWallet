#!/bin/bash
#
# Quick start script for RNG data capture testing
# Usage: ./RNG_DUMP_QUICK_START.sh
#
# Prerequisites:
#   - Firmware compiled with: make USE_RNG_DUMP=1
#   - Binary at: build/cryptowallet.bin
#   - st-flash installed (apt install stlink-tools)
#   - Python venv activated: source .venv-test/bin/activate
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FIRMWARE="${SCRIPT_DIR}/build/cryptowallet.bin"
PORT="${1:-/dev/ttyACM0}"
BAUD="115200"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}   RNG Data Capture Quick Start${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Check firmware
if [ ! -f "$FIRMWARE" ]; then
    echo -e "${RED}✗ Firmware not found: $FIRMWARE${NC}"
    echo "  Build with: make USE_RNG_DUMP=1"
    exit 1
fi
echo -e "${GREEN}✓ Firmware found: $FIRMWARE${NC}"
echo "  Size: $(ls -lh $FIRMWARE | awk '{print $5}')"
echo ""

# Check st-flash
if ! command -v st-flash &> /dev/null; then
    echo -e "${RED}✗ st-flash not found${NC}"
    echo "  Install: sudo apt install stlink-tools"
    exit 1
fi
echo -e "${GREEN}✓ st-flash found${NC}"
echo ""

# Flash firmware
echo -e "${YELLOW}STEP 1: Flashing firmware to STM32H743ZI2${NC}"
echo "This will take ~10-30 seconds..."
echo ""

if st-flash --freq=24000 write "$FIRMWARE" 0x08000000; then
    echo -e "${GREEN}✓ Firmware flashed successfully${NC}"
else
    echo -e "${RED}✗ Failed to flash firmware${NC}"
    echo "  - Ensure device is connected via USB ST-Link"
    echo "  - Try: st-info --probe"
    exit 1
fi
echo ""

# Wait for device to boot
echo -e "${YELLOW}STEP 2: Device boot (waiting 2 seconds)...${NC}"
sleep 2
echo -e "${GREEN}✓ Device should be ready${NC}"
echo ""

# Check port
echo -e "${YELLOW}STEP 3: Checking UART port${NC}"
if [ ! -c "$PORT" ]; then
    echo -e "${YELLOW}⚠ Port $PORT not found${NC}"
    echo "  Available ports:"
    ls -1 /dev/tty* | grep -E "ACM|USB" | sed 's/^/    /'
    echo ""
    echo "  Use: $0 /dev/ttyACM1  (or other port)"
    exit 1
fi
echo -e "${GREEN}✓ UART port ready: $PORT${NC}"
echo ""

# Start capture
echo -e "${YELLOW}STEP 4: Starting RNG data capture${NC}"
echo "Collecting 128 MiB of RNG data (this takes ~5 minutes)..."
echo "  Command: python3 scripts/test_rng_signing_comprehensive.py --mode rng --port $PORT"
echo ""

source "${SCRIPT_DIR}/.venv-test/bin/activate" 2>/dev/null || {
    echo -e "${YELLOW}⚠ Virtual environment not activated${NC}"
    echo "  Run: source .venv-test/bin/activate"
}

python3 "${SCRIPT_DIR}/scripts/test_rng_signing_comprehensive.py" --mode rng --port "$PORT"

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ RNG capture completed successfully${NC}"
    echo ""
    echo "Output files:"
    ls -lh rng.bin dieharder_results_*.txt 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
    echo ""
    echo "Next steps:"
    echo "  1. Run Dieharder tests:"
    echo "     dieharder -a -g 201 -f rng.bin"
    echo "  2. View summary:"
    echo "     grep 'PASSED\|FAILED\|WEAK' dieharder_results_*.txt"
else
    echo -e "${RED}✗ RNG capture failed${NC}"
    exit 1
fi
