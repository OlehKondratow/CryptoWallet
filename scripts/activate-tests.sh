#!/bin/bash
# Activate Python virtual environment for CryptoWallet testing (run from any cwd).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ ! -d "$REPO_ROOT/.venv-test" ]; then
    echo "Error: .venv-test not found at $REPO_ROOT/.venv-test"
    exit 1
fi

# shellcheck source=/dev/null
source "$REPO_ROOT/.venv-test/bin/activate"

echo ""
echo "✓ Virtual environment activated"
echo "  Python: $(python3 --version)"
echo "  Executable: $(which python3)"
echo "  Repo root: $REPO_ROOT"
echo ""
echo "Available commands:"
echo "  python3 scripts/test_rng_signing_comprehensive.py --help"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode rng"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode dieharder"
echo "  python3 scripts/test_rng_signing_comprehensive.py --mode signing"
echo ""
echo "To deactivate: deactivate"
echo ""
