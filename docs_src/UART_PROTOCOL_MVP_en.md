\page uart_protocol_mvp_en "UART protocol MVP (CWUP-0.1)"
\related CI_PIPELINE_en
\related RNG_THREAT_MODEL_P2_RU

# CWUP-0.1 — CryptoWallet UART Protocol (MVP)

<brief>Minimal UART3 protocol for **lab diagnostics**, **CI**, and **framed TRNG capture** with command/binary separation. It does **not** replace the primary user channel (HTTP/WebUSB) and **does not** define firmware update over UART — that belongs to the bootloader (`stm32_secure_boot`) and separate policy.</brief>

---

## 1. Scope

| In MVP | Out of scope |
|--------|----------------|
| Text command channel after app start | ROM / ST-Link bootloader protocol |
| Framed TRNG for dieharder | DFU/OTA, image upload over UART |
| CI readiness lines (`CW+READY`, markers) | Transaction signing over UART (see §6) |
| Single TX discipline (queue/mutex) | UART link encryption |

**Protocol version string:** `CWUP/0.1` (in `AT+CWINFO?`).

---

## 2. Boot chain and trust (relationship to CWUP)

```
[STM32 ROM] → (optional) [Secure bootloader @ stm32_secure_boot] → [CryptoWallet app @ 0x0800…]
```

- **Bootloader protection and image authentication** are **outside** CWUP; defined by `stm32_secure_boot` and linker layout.
- **CWUP applies only after** the application runs (post-`main`, `HW_Init`, FreeRTOS scheduler).
- While the **bootloader** runs, the host **must not** treat traffic as CWUP until the app preamble (§4) appears.

---

## 3. Task startup and readiness

After `osKernelStart()`: Display, Net, Sign, IO, User, (optional) RNG.

**MVP readiness phases:**

1. **Phase A:** until `CW+READY`, no TRNG **frames**; optional TEXT logs only.
2. **Phase B:** single line:  
   `CW+READY,proto=CWUP/0.1,build=<git-describe|unknown>,rng=1|0\r\n`
3. **Phase C:** host sends commands (§5); TRNG frames only after `AT+RNG=START`.

---

## 4. Line modes

| Mode | TX | RX |
|------|----|----|
| TEXT | Lines ending `\r\n` | `AT+...` |
| BINARY_RNG | Frames §7 only | `AT+RNG=STOP` or timeout |

**T→B:** only after `AT+RNG=START` and `CW+RNG OPEN`.  
**B→T:** `AT+RNG=STOP`, session timeout, RNG error, or reset.

**Rule:** no `printf` logs on the same UART in mode **B**.

---

## 5. Commands (MVP)

Lines: `AT+<VERB>[=<args>]\r\n`. Responses: `CW+<CODE>[,payload]\r\n` or `CW+ERR=<n>,<msg>\r\n`.

| Command | Purpose |
|---------|---------|
| `AT+CWINFO?` | Protocol + firmware info |
| `AT+FWINFO?` | **Application image integrity** in Flash: CRC32 (IEEE), length, region bounds — match against `scripts/fw_integrity_check.py` and `build/cryptowallet.bin` (lab / regression) |
| `AT+BOOTCHAIN?` | **Human-readable boot chain** known to the app: verified bootloader presence (`stm32_secure_boot`), app entry. Does not replace ROM signature checks |
| `AT+MARKS` | Emit lines compatible with `uart_boot_markers.txt` (CI) |
| `AT+RNG=START,<total_bytes>` | Open framed TRNG session |
| `AT+RNG=STOP` | Close session, back to TEXT |
| `AT+PING` | `CW+PONG` |
| `AT+READY?` | Duplicate readiness line (`CW+READY,...`) |
| `AT+WALLET?` | **Lab / CI (no authentication):** `seed=0|1` from `get_wallet_seed()` probe, `crypto_sign=0|1` from build. Does **not** export keys |
| `AT+SELFTEST?` | `CW+SELFTEST,ok=1,tick=<FreeRTOS ticks>` |
| `AT+ECHO=<text>` | `CW+ECHO,<text>` printable ASCII only (≤120 chars) — pipeline regression |

**Example responses:**

- `AT+FWINFO?` → `CW+FWINFO,FWINFO crc32=0x...,len=...,start=0x...,end=0x...` (same semantics as startup log / `fw_integrity_snprint()`).
- `AT+BOOTCHAIN?` → `CW+BOOTCHAIN,rom=STM32,verified_boot=0|1,note=...` (build-dependent; if unknown → `CW+ERR=12,bootchain unknown`).
- `AT+WALLET?` → `CW+WALLET,seed=0|1,crypto_sign=0|1` (`seed=1` when e.g. `USE_TEST_SEED=1`).
- `AT+SELFTEST?` → `CW+SELFTEST,ok=1,tick=...`
- `AT+ECHO=hi` → `CW+ECHO,hi`

---

## 6. Wallet operations vs channels

| Operation | Product channel | CWUP-0.1 |
|-----------|-----------------|----------|
| Liveness / diag | UART (opt.), OLED | `AT+PING`, `CW+READY` |
| TRNG capture | UART | `AT+RNG=START` + frames |
| Boot markers | UART | `AT+MARKS` |
| Runtime FW integrity | UART | `AT+FWINFO?` + host `fw_integrity_check.py` |
| Boot chain (lab) | UART | `AT+BOOTCHAIN?` + `stm32_secure_boot` docs |
| Status / TX signing | HTTP / WebUSB | unchanged |
| FW update | Bootloader / ST-Link | **not CWUP** |

Read-only `AT+WALLET?` / `AT+SELFTEST?` / `AT+ECHO=` are **lab/CI** helpers (no auth). Write-style wallet commands remain on HTTP/WebUSB — **not** CWUP-0.1.

---

## 7. TRNG frame (MVP)

```
[0..1]   magic 0xC7 0x57
[2..3]   u16 payload_len N (LE, N ≤ 256)
[4..3+N] N bytes TRNG
[4+N..7+N] u32 CRC32 (IEEE) over [0 .. 4+N-1]
```

Host extracts payload bytes only; validates CRC per frame.

---

## 8. Timeouts (recommended)

| Parameter | Value |
|-----------|-------|
| Idle after `CW+READY` | 120 s |
| RNG session stall | 180 s |

---

## 9. Repo tests mapping

| Script | CWUP behavior |
|--------|----------------|
| `uart_wait_boot_log.py` | Wait for `CW+READY` / `AT+MARKS` |
| `capture_rng_uart.py` | Frame parser §7 |
| `simple-ci.yml` | READY → MARKS → RNG → dieharder |
| `bootloader_secure_signing_test.py` | Unchanged (HTTP) |

---

## 10. Implementation status (firmware)

- **Implemented** when **`USE_RNG_DUMP` is off**: USART3 RX (`HAL_UART_Receive_IT`), line assembly, **`CWUP`** task, shared TX mutex with logs (and with RNG blocks when `USE_RNG_DUMP=1`). Commands: `AT+PING`, `AT+CWINFO?`, `AT+READY?`, `AT+FWINFO?`, `AT+BOOTCHAIN?`, `AT+WALLET?`, `AT+SELFTEST?`, `AT+ECHO=`; on task start: **`CW+READY,...`** line.
- **Disabled when `USE_RNG_DUMP=1`:** binary TRNG on the same UART — build without this flag for text CWUP.

## 11. Future (v0.2+)

- `AT+RNG=START` / framed mode B, `AT+MARKS` — not in firmware yet.
- `CWUP_LEGACY_RAW` for old raw `USE_RNG_DUMP` stream.

**Русский:** [UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md)
