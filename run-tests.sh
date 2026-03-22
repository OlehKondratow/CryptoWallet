#!/bin/bash
# Quick test runner - activates venv and runs scripts/test_rng_signing_comprehensive.py
#
# Примеры:
#   ./run-tests.sh --mode verify-all --quick --skip-signing
#   ./run-tests.sh --mode analyze --file rng.bin
# Полный лабораторный RNG-конвейер (отдельный скрипт):
#   ./scripts/run_full_rng_verification.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Activate venv if not already active
if [ -z "$VIRTUAL_ENV" ]; then
  source .venv-test/bin/activate
fi

# Run the test script with all arguments
python3 scripts/test_rng_signing_comprehensive.py "$@"
