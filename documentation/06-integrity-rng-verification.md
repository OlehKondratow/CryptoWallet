# 6. Integrity, RNG, and verification

## 6.1 Runtime firmware integrity (`fw_integrity`)

**Purpose:** Compare CRC32 and length of the **running** application region against expectations—detect corruption or wrong image. **Not** a substitute for verified boot.

| Item | Location |
|------|----------|
| Implementation | `Core/Src/fw_integrity.c`, `Core/Inc/fw_integrity.h` |
| Region | Linker symbols `__app_flash_start` / `__app_flash_end` |
| Log line | `FWINFO crc32=…,len=…` at startup |
| Host check | `scripts/fw_integrity_check.py` on `build/cryptowallet.bin`; optional `--expect-fwinfo-line` from UART |

**CWUP:** `AT+FWINFO?` returns the same string when CWUP is active (`USE_RNG_DUMP=0`).

## 6.2 Two UART TRNG modes (do not mix)

| Mode | Build | UART content |
|------|-------|----------------|
| Raw dump | `USE_RNG_DUMP=1` | Continuous binary TRNG bytes — **CWUP disabled** |
| Framed (spec) | CWUP §7 | `AT+RNG=START` then frames — **not implemented** in firmware yet |

`scripts/capture_rng_uart.py` captures **raw** bytes for dieharder-style analysis when `USE_RNG_DUMP=1`.

**Cryptographic use vs this capture path:** UART dump exists to **verify** the TRNG statistically. **Provisioning** a future encrypted wallet blob (salts, AEAD nonces) must use the **same hardware RNG inside firmware**, not the host file produced by capture—see **section 3.5** of [`03-cryptography-and-signing.md`](03-cryptography-and-signing.md) (TRNG role, UID binding, `USE_RNG_DUMP` scope).

## 6.3 Host tests (no board)

- **`pytest tests/mvp`** — CRC/build checks, FWINFO parsing, secure boot image tamper logic (`ecdsa` from `requirements-test.txt`).
- **`scripts/fw_integrity_check.py`** — CI step “FW integrity”.

## 6.4 Board / CI

- **Gitea:** `.gitea/workflows/simple-ci.yml` — build, flash, UART analysis, **RNG capture** when `CI_BUILD_USE_RNG_DUMP=1` and not skipped.
- **Env highlights:** `CRYPTO_DEPS_ROOT` (sibling `STM32CubeH7`, `stm32_secure_boot`, `stm32-ssd1306`), `CI_RNG_CAPTURE_BYTES` (default large capture for statistics), `CI_RNG_UART_CAPTURE_STRICT=0|1` (whether failed capture fails the job), `CI_SKIP_RNG_UART_CAPTURE`.

## 6.5 Dieharder (quick smoke)

CI may run `dieharder` on a captured file. **Exit code 0** is normal even when a single test row shows `FAILED`—that is one statistic, not process failure. Short files are **rewound** many times; **p-values are not interpretable like an infinite i.i.d. stream**. For serious review: large capture (hundreds of MiB) and extended dieharder, offline.

## 6.6 Manual checklist

1. Build → `build/cryptowallet.bin`.
2. Flash → UART log contains `FWINFO`.
3. `python3 scripts/fw_integrity_check.py build/cryptowallet.bin` — match CRC/length to device log or `AT+FWINFO?` when applicable.
