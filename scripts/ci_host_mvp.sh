#!/usr/bin/env bash
# Host-only checks aligned with .gitea/workflows (build job): fw_integrity_check + pytest tests/mvp.
# Run from repository root after: make  →  build/cryptowallet.bin
#
# Usage:
#   ./scripts/ci_host_mvp.sh
#   PYTHON=python3.11 ./scripts/ci_host_mvp.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PY="${PYTHON:-python3}"
BIN="${CW_HOST_BIN:-build/cryptowallet.bin}"

if [[ ! -f "$BIN" ]]; then
  echo "ERROR: $BIN not found. Run: make" >&2
  exit 2
fi

echo "== 1/2 fw_integrity_check.py =="
"$PY" scripts/fw_integrity_check.py "$BIN"

echo "== 2/2 pytest tests/mvp =="
if ! "$PY" -m pytest --version >/dev/null 2>&1; then
  echo "ERROR: pytest not available for $PY. Example: python3 -m venv .venv && .venv/bin/pip install -r requirements-test.txt && PYTHON=.venv/bin/python $0" >&2
  exit 2
fi
export STM32_SECURE_BOOT="${STM32_SECURE_BOOT:-${CRYPTO_DEPS_ROOT:-/data/projects}/stm32_secure_boot}"
"$PY" -m pytest tests/mvp -q --tb=short
echo "OK: host MVP checks passed"
