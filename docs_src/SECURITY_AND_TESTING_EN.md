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

---

## Runtime integrity (`fw_integrity`)

- Implementation: `Core/Src/fw_integrity.c`, linker symbols in `STM32H743ZITx_FLASH_LWIP.ld`.
- Host check: `scripts/fw_integrity_check.py` on `build/cryptowallet.bin`.
- Not a substitute for verified boot.

---

## CWUP commands (lab)

- `AT+FWINFO?` — CRC32 / bounds (match `fw_integrity_snprint()`).
- `AT+BOOTCHAIN?` — textual boot chain hints for the lab.

Full RX may be pending; use startup **`FWINFO`** log line until then.

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
