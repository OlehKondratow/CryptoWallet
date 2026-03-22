\page uart_protocol_mvp_en "UART protocol MVP (CWUP-0.1)"
\related CI_PIPELINE_en
\related RNG_THREAT_MODEL_P2_RU

# CWUP-0.1 — CryptoWallet UART Protocol (MVP)

<brief>Minimal UART3 protocol for **lab diagnostics**, **CI**, and **framed TRNG capture** with command/binary separation. It does **not** replace the primary user channel (HTTP/WebUSB) and **does not** define firmware update over UART — that belongs to the bootloader (`stm32_secure_boot`) and separate policy.</brief>

**Default build (`make` / `minimal-lwip`):** `USE_WEBUSB=1`, `USE_RNG_DUMP=0` — **WebUSB** (PA11/PA12) and **MVP CWUP** on USART3 together. `USE_RNG_DUMP=1` disables CWUP on UART (binary TRNG stream); disable USB if needed: `USE_WEBUSB=0`.

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

**Line phases (keep distinct in firmware, tests, and CI — do not conflate with raw `USE_RNG_DUMP=1`):**

| Phase | Meaning | Device TX | Notes |
|-------|---------|-----------|-------|
| **A** | Startup before CWUP readiness | *TEXT* only (`\r\n`); **no** §7 TRNG frames | Other tasks may log on the same UART (app preamble after bootloader). Host treats the link as **not** CWUP-ready until phase B. |
| **B** | Readiness announcement | One *TEXT* line: `CW+READY,proto=CWUP/0.1,build=<git-describe|unknown>,rng=0|1\r\n` | `rng=1` means framed TRNG via CWUP (`AT+RNG=START`, §7) is available in **this** build — **not** the raw `USE_RNG_DUMP` stream. |
| **C** | Commands, TEXT mode (T) | `CW+...` replies to `AT+...` (§5) | Until a successful `AT+RNG=START`, no §7 frames; line stays TEXT. Typical CI: wait for `CW+READY` → optional markers → further AT commands. |
| **D** | Binary TRNG session (protocol T→B) | §7 frames only on TX | Starts after `AT+RNG=START` and a response such as `CW+RNG OPEN` (**not implemented** in firmware yet). In D, `printf` logs on that UART are forbidden without coordinated TX locking. Exit: `AT+RNG=STOP`, timeout, RNG error, reset → back to TEXT (phase C). |

Firmware `Core/Src/cwup_uart.c` tracks **A→B→C** around the first `CW+READY` line; phase **D** is reserved until `AT+RNG=START` is implemented.

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

Text CWUP requires **`USE_RNG_DUMP` unset** at build. RX: USART3 (`HAL_UART_Receive_IT`), lines to `\r\n`, **`CWUP`** task, shared TX mutex with logs (`UART_Tx_Lock`). When **`USE_RNG_DUMP=1`**, the same UART carries raw binary TRNG and **`Cwup_Init()`** does not start CWUP (see §11).

| Command / item | Status | Notes |
|----------------|--------|-------|
| **`CW+READY,...`** on startup | Implemented | Phase B |
| **`AT+PING`** | Implemented | `CW+PONG` |
| **`AT+CWINFO?`** | Implemented | protocol, build |
| **`AT+READY?`** | Implemented | duplicate `CW+READY` |
| **`AT+FWINFO?`** | Implemented | `fw_integrity_snprint()` |
| **`AT+BOOTCHAIN?`** | Implemented | lab, macros + `VTOR` |
| **`AT+WALLET?`** | Implemented | lab / CI, no secrets |
| **`AT+SELFTEST?`** | Implemented | FreeRTOS tick |
| **`AT+ECHO=`** | Implemented | pipeline regression |
| **`AT+MARKS`** | Not implemented | spec §5 only |
| **`AT+RNG=START`**, **`AT+RNG=STOP`**, §7 frames | Not implemented | phase D; not the same as raw `USE_RNG_DUMP` |

## 11. Future (v0.2+)

- **`AT+MARKS`** and **framed TRNG** (§7, `AT+RNG=START`) — see §10 table.
- `CWUP_LEGACY_RAW` idea for old raw `USE_RNG_DUMP` stream (not in firmware).

**Русский:** [UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md)
