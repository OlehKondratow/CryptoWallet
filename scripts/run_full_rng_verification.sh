#!/usr/bin/env bash
# Полная лабораторная проверка RNG: захват (capture_rng_uart.py) + анализ + dieharder + отчёт.
# Требует: прошивка с USE_RNG_DUMP=1, устройство на CI_UART_PORT (по умолчанию /dev/ttyACM0).
#
# Использование:
#   ./scripts/run_full_rng_verification.sh
#   RNG_CAPTURE_BYTES=134217728 ./scripts/run_full_rng_verification.sh
#   QUICK=1 ./scripts/run_full_rng_verification.sh   # 1 MiB + один тест dieharder
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

OUT_BIN="${RNG_OUT_FILE:-rng_full_verify.bin}"
REPORT="${RNG_REPORT_FILE:-rng_full_verify_report.txt}"
PORT="${CI_UART_PORT:-/dev/ttyACM0}"
BYTES="${RNG_CAPTURE_BYTES:-134217728}"
QUICK="${QUICK:-0}"

if [[ "$QUICK" == "1" ]]; then
  BYTES="${RNG_CAPTURE_BYTES:-20000000}"
fi

echo "═══════════════════════════════════════════════════════════════════"
echo " CryptoWallet — полная проверка RNG (лабораторный сценарий)"
echo "═══════════════════════════════════════════════════════════════════"
echo "Репозиторий: $REPO_ROOT"
echo "Порт: $PORT  |  байт захвата: $BYTES"
echo ""
echo "Перед запуском:"
echo "  1) make clean && make USE_RNG_DUMP=1 … && make flash"
echo "  2) На UART — только бинарный поток (без текстовых логов)."
echo ""

{
  echo "=== CryptoWallet RNG full verification ==="
  echo "Date UTC: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "Host: $(uname -a)"
  echo "Git: $(git -C "$REPO_ROOT" describe --always --dirty 2>/dev/null || echo 'n/a')"
  echo "Port: $PORT  bytes: $BYTES  QUICK=$QUICK"
  echo ""
} | tee "$REPORT"

if [[ ! -e "$PORT" ]]; then
  echo "ERROR: serial port not found: $PORT" | tee -a "$REPORT"
  exit 1
fi

if ! command -v dieharder >/dev/null 2>&1; then
  echo "WARN: dieharder not in PATH — установите пакет dieharder для фазы 3." | tee -a "$REPORT"
fi

echo ">>> Phase 1: capture (scripts/capture_rng_uart.py)" | tee -a "$REPORT"
set +e
python3 "$REPO_ROOT/scripts/capture_rng_uart.py" \
  --port "$PORT" \
  --out "$OUT_BIN" \
  --bytes "$BYTES" \
  2>&1 | tee -a "$REPORT"
CAP_RC=$?
set -e

SIZE=$(stat -c%s "$OUT_BIN" 2>/dev/null || stat -f%z "$OUT_BIN" 2>/dev/null || echo 0)
echo "Captured file size: $SIZE bytes" | tee -a "$REPORT"
if [[ "$CAP_RC" -ne 0 ]] || [[ "$SIZE" -lt 1000 ]]; then
  echo "ERROR: capture failed or file too small (rc=$CAP_RC)" | tee -a "$REPORT"
  exit 2
fi

echo "" | tee -a "$REPORT"
echo ">>> Phase 2: RNG quality analysis" | tee -a "$REPORT"
set +e
python3 "$REPO_ROOT/scripts/test_rng_signing_comprehensive.py" \
  --mode analyze \
  --file "$OUT_BIN" \
  2>&1 | tee -a "$REPORT"
AN_RC=$?
set -e
if [[ "$AN_RC" -ne 0 ]]; then
  echo "WARN: analyze exited with $AN_RC" | tee -a "$REPORT"
fi

echo "" | tee -a "$REPORT"
echo ">>> Phase 3: dieharder (через test_rng_signing_comprehensive.py)" | tee -a "$REPORT"
if ! command -v dieharder >/dev/null 2>&1; then
  echo "Skip dieharder (not installed)." | tee -a "$REPORT"
  exit 0
fi

set +e
if [[ "$QUICK" == "1" ]]; then
  python3 "$REPO_ROOT/scripts/test_rng_signing_comprehensive.py" \
    --mode dieharder \
    --file "$OUT_BIN" \
    --quick \
    2>&1 | tee -a "$REPORT"
else
  python3 "$REPO_ROOT/scripts/test_rng_signing_comprehensive.py" \
    --mode dieharder \
    --file "$OUT_BIN" \
    2>&1 | tee -a "$REPORT"
fi
DH_RC=$?
set -e

echo "" | tee -a "$REPORT"
echo "dieharder phase exit code: $DH_RC" | tee -a "$REPORT"
echo "Done. Report: $REPORT  |  binary: $OUT_BIN" | tee -a "$REPORT"
exit "$DH_RC"
