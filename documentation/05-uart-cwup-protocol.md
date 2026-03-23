# 5. UART — CWUP-0.1 (CryptoWallet UART protocol)

**Implementation:** `Core/Src/cwup_uart.c`. **Default product build:** `USE_WEBUSB=1`, `USE_RNG_DUMP=0` — CWUP on USART3 together with WebUSB on PA11/PA12.

**Scope:** Lab diagnostics, CI, and (specified) framed TRNG. **Not** firmware update over UART (bootloader owns that). **Not** a replacement for HTTP/WebUSB as the primary user channel.

## 5.1 Boot chain context

```
ROM → (optional stm32_secure_boot) → CryptoWallet application
```

CWUP exists **only after** FreeRTOS is running. Traffic during bootloader execution is **not** CWUP.

## 5.2 Line phases

| Phase | Meaning | Host view |
|-------|---------|-----------|
| A | Before CWUP ready | Text may appear; not CWUP-ready |
| B | `CW+READY,proto=CWUP/0.1,...` | CWUP announced |
| C | TEXT mode: `AT+` / `CW+` lines | Commands per §5.4 |
| D | Binary framed TRNG (§5.5) | After `AT+RNG=START` — **not implemented** in firmware yet |

`rng=` in `CW+READY` refers to **framed** TRNG availability — **not** the raw `USE_RNG_DUMP` stream.

## 5.3 Modes

| Mode | TX | RX |
|------|----|----|
| TEXT | Lines `\r\n` | `AT+...` |
| BINARY_RNG | Frames §5.5 only | `AT+RNG=STOP` / timeout |

Rule: no `printf` logs on the same UART in BINARY_RNG.

## 5.4 Commands (MVP)

Format: `AT+<VERB>[=<args>]\r\n` → `CW+<CODE>[,payload]\r\n` or `CW+ERR=<n>,<msg>\r\n`.

| Command | Purpose |
|---------|---------|
| `AT+CWINFO?` | Protocol + build info |
| `AT+FWINFO?` | App integrity string (matches `fw_integrity_snprint()` / log `FWINFO`) |
| `AT+BOOTCHAIN?` | Human-readable boot chain (lab) |
| `AT+MARKS` | CI markers — **not implemented** |
| `AT+RNG=START,<bytes>` / `AT+RNG=STOP` | Framed TRNG session — **not implemented** |
| `AT+PING` | `CW+PONG` |
| `AT+READY?` | Repeat `CW+READY` |
| `AT+WALLET?` | Lab: `seed` / `crypto_sign` flags — **no secrets** |
| `AT+SELFTEST?` | Tick / sanity |
| `AT+ECHO=<text>` | Printable ASCII — CI regression |

Wallet **writes** stay on HTTP/WebUSB—not CWUP-0.1.

## 5.5 TRNG frame format (specified; host parser)

```
[0..1]   magic 0xC7 0x57
[2..3]   u16 payload_len N (LE, N ≤ 256)
[4..3+N] N bytes TRNG
[4+N..7+N] u32 CRC32 (IEEE) over [0 .. 4+N-1]
```

## 5.6 Implementation status (firmware)

| Item | Status |
|------|--------|
| `CW+READY`, `AT+PING`, `AT+CWINFO?`, `AT+FWINFO?`, `AT+BOOTCHAIN?`, `AT+WALLET?`, `AT+SELFTEST?`, `AT+ECHO=` | Implemented |
| `AT+MARKS`, framed TRNG §5.5 + `AT+RNG=*` | **Not implemented** |
| `USE_RNG_DUMP=1` | Raw binary UART — **Cwup_Init** does not start CWUP |

## 5.7 Scripts mapping

| Script | Role |
|--------|------|
| `uart_wait_boot_log.py` | Wait for `CW+READY` / markers when text mode |
| `scripts/test_cwup_mvp.py` | HIL AT tests |
| `scripts/capture_rng_uart.py` | Raw byte capture for **dump** mode — not §5.5 frame parser today |
