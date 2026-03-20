#!/bin/bash
# Activate Python virtual environment for CryptoWallet testing

# Check if running from correct directory
if [ ! -d ".venv-test" ]; then
    echo "Error: .venv-test not found. Are you in CryptoWallet root?"
    exit 1
fi

# Activate virtual environment
source .venv-test/bin/activate

# Show status
echo ""
echo "✓ Virtual environment activated"
echo "  Python: $(python3 --version)"
echo "  Executable: $(which python3)"
echo "  Location: $(pwd)"
echo ""
echo "Available commands:"
echo "  python3 scripts/test_rng_signing_comprehensive.py --help"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode rng"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode dieharder"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode signing"
echo ""
echo "To deactivate: deactivate"
echo ""
