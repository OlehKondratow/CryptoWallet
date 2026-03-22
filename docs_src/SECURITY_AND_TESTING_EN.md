\page security_and_testing_en "Security and testing (overview)"
\related CI_PIPELINE_en
\related UART_PROTOCOL_MVP_en
\related INFRASTRUCTURE

# CryptoWallet — security and testing (overview)

<brief>Single entry point for **trust model**, **runtime firmware integrity**, **UART/CWUP lab diagnostics**, **RNG**, and **CI**. Details remain in the linked documents.</brief>

---

## Purpose

This complements scattered docs (`CI_PIPELINE_*`, `RNG_*`, `UART_PROTOCOL_MVP_*`, `TESTING_GUIDE_*`, bootloader `stm32_secure_boot`). **Authoritative deep-dive remains in those files.**

**Russian (full text):** [SECURITY_AND_TESTING_RU.md](SECURITY_AND_TESTING_RU.md)

---

## Boot chain

```
[STM32 ROM] → (optional) [Verified bootloader @ stm32_secure_boot] → [CryptoWallet app]
```

- Bootloader signature / TrustZone: project **`stm32_secure_boot`**, not this app repo.
- **CWUP** applies only **after** the app starts ([UART_PROTOCOL_MVP_en.md](UART_PROTOCOL_MVP_en.md)).

**Signing model (not X.509):** in **`stm32_secure_boot`**, the application image is signed with **ECDSA secp256k1** and a PEM key (`scripts/sign_image.py`) — **not** an X.509 / PKCS#7 chain. Host-side format checks: **`scripts/secure_boot_image.py`**, **`tests/mvp/test_secure_boot_sign_tamper.py`** (needs `ecdsa` from `requirements-test.txt`). Those tests validate the **signed app image format**, not flashing separate bootloader + app slots.

---

## Runtime integrity (`fw_integrity`)

- Implementation: `Core/Src/fw_integrity.c`, linker symbols `__app_flash_start` / `__app_flash_end` in `STM32H743ZITx_FLASH_LWIP.ld`.
- Host: `scripts/fw_integrity_check.py` on `build/cryptowallet.bin`. Optional **`--expect-fwinfo-line`** (full `AT+FWINFO?` / log line) checks CRC **and** file length vs `len=`. The printed CRC matches the device only if the **file bytes** are the same image as the contiguous Flash range `[start, end)` on the device — see the script docstring and [SECURITY_AND_TESTING_RU.md](SECURITY_AND_TESTING_RU.md) §3.
- Not a substitute for verified boot.

---

## CWUP (lab UART)

Protocol: [UART_PROTOCOL_MVP_en.md](UART_PROTOCOL_MVP_en.md).

**Authoritative command implementation table:** [UART_PROTOCOL_MVP_en.md](UART_PROTOCOL_MVP_en.md) §10.

- `AT+FWINFO?` — same string as `fw_integrity_snprint()` / `FWINFO` log.
- `AT+BOOTCHAIN?` — lab hints (build macros, `VTOR`); not a ROM attestation.

**When `USE_RNG_DUMP=1`:** raw binary TRNG on the same UART — text CWUP is **disabled**.

**Two UART TRNG modes (do not mix):** **`USE_RNG_DUMP=1`** = continuous **raw** TRNG stream, CWUP off. **Framed** TRNG per CWUP spec (§7, `AT+RNG=START` / `CW+RNG OPEN`) is **not implemented** yet in `cwup_uart.c` — it is **not** the same as the raw dump.

---

## RNG and signing

- [RNG_SECURITY_IMPROVEMENTS_RU.md](RNG_SECURITY_IMPROVEMENTS_RU.md) (recommendations)
- [RNG_THREAT_MODEL_P2_RU.md](RNG_THREAT_MODEL_P2_RU.md) (threat model P2)
- [TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md) — `scripts/test_rng_signing_comprehensive.py`

---

## HTTP API (signing over LAN)

- [HTTP_API_en.md](HTTP_API_en.md) (`Src/task_net.c`)

---

## CI

- [CI_PIPELINE_en.md](CI_PIPELINE_en.md), `.gitea/workflows/simple-ci.yml`
- [INFRASTRUCTURE.md](INFRASTRUCTURE.md)
