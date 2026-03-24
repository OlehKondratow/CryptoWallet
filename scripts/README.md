# Scripts (host)

![Project photo](../images/photo_2026-03-23_14-41-33.jpg)

**Manual:** [`documentation/README.md`](../documentation/README.md) — security model, CI, UART, integrity.

## CI / regression

| Script | Role |
|--------|------|
| `fw_integrity_check.py` | Compare `build/cryptowallet.bin` to `FWINFO` / `AT+FWINFO?` |
| `capture_rng_uart.py` | Raw UART capture for TRNG statistics (`USE_RNG_DUMP=1`) |
| `test_cwup_mvp.py` | CWUP HIL (`USE_RNG_DUMP=0`) |
| `ci_host_mvp.sh` | Local host checks mirroring CI |

## Generators (called from `make`)

```bash
make docs-md          # documentation/generated/{reference-code,testing-plan-signing-rng}.md
make docs             # + MkDocs → site/
make docs-doxygen     # Doxygen + README Project Structure table
```

## Python environment

```bash
pip install -r requirements-test.txt
```

| Shell helper | Role |
|--------------|------|
| `scripts/install-test-deps.sh` | System `dieharder` + venv packages (`requirements-test.txt`) |
| `scripts/activate-tests.sh` | `source` the repo `.venv-test` |
| `scripts/run-tests.sh` | Wrapper → `test_rng_signing_comprehensive.py` |
| `scripts/RNG_DUMP_QUICK_START.sh` | Flash RNG-dump build + capture |
| `scripts/RNG_SETUP_QUICK_COMMANDS.sh` | Lab steps (Makefile / build / flash) |

See [`documentation/07-build-ci-infrastructure.md`](../documentation/07-build-ci-infrastructure.md).
