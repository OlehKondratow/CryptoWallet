# CryptoWallet

STM32H743 firmware: Bitcoin-oriented signing, FreeRTOS, LwIP (HTTP), WebUSB, UART diagnostics (CWUP). **Canonical documentation** lives in [`documentation/README.md`](documentation/README.md) — one structured manual (trust model → build/CI) with uniform depth.

**Security note:** HTTP has no TLS; UART CWUP has no authentication. Bootloader verification is in repo `stm32_secure_boot`, not here. See documentation §1.

## Build

Requires sibling trees `STM32CubeH7`, `stm32_secure_boot`, `stm32-ssd1306` (or set `CRYPTO_DEPS_ROOT` to their parent directory):

```bash
export CRYPTO_DEPS_ROOT=/data/projects   # example
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" clean all
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" flash
```

## Tests (host)

```bash
pip install -r requirements-test.txt
pytest tests/mvp
```

## Documentation site (optional)

```bash
python3 -m venv .venv-docs && .venv-docs/bin/pip install -r requirements-docs.txt
make docs
# open site/index.html
```

## Project Structure

| Module | Brief |
|--------|-------|
| *(run `make docs-doxygen` to refresh this table from source headers)* | |

## Languages

- [Русский — кратко](README_ru.md)
- [Polski — krótko](README_pl.md)
