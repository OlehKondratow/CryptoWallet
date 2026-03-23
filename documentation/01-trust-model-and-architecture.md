# 1. Trust model and architecture

## 1.1 System picture

CryptoWallet is **application firmware** on STM32H743. It does not replace ROM, option bytes, or a verified bootloader. Security claims must be stated **per layer**:

```
STM32 ROM  →  (optional) verified bootloader @ stm32_secure_boot  →  CryptoWallet @ Flash
```

| Layer | Repository / owner | What it proves |
|--------|---------------------|----------------|
| ROM / debug lock | ST, board config | Debug access, RDP, vectors |
| Bootloader | `stm32_secure_boot` (separate repo) | Authenticity of the app image **before** jump to app |
| Application | **this repo** | Wallet logic, runtime self-checks, UART/HTTP/USB **surfaces** |

Anything that requires **ROM or bootloader cryptography** is **out of scope** for CWUP and HTTP documentation in this repo—they only run **after** `main()` and scheduler start.

## 1.2 Application responsibilities (this repo)

- **Transaction signing** with user confirmation on-device (button), not over the network.
- **Optional runtime firmware integrity** (`fw_integrity`): CRC/length over the linked app region—detects accidental corruption; **not** verified boot.
- **Diagnostics**: UART CWUP (when `USE_RNG_DUMP=0`), HTTP (port 80), WebUSB—each has different trust assumptions (see chapters 4–6).

## 1.3 Surfaces (attack / operator view)

| Surface | Confidentiality | Integrity | Notes |
|---------|-----------------|-----------|--------|
| HTTP :80 | None (no TLS) | Best-effort parsing | Trusted LAN **only**; user confirms on OLED |
| WebUSB | Depends on browser + cable | Framed binary protocol | Product path alongside HTTP |
| UART3 CWUP | No encryption | Line-at-a-time AT commands | Lab/CI; **no** auth on `AT+WALLET?` etc. |
| UART3 `USE_RNG_DUMP=1` | N/A | Raw bytes for statistics | **Disables** CWUP on that UART |

## 1.3b Authentication and authorization (explicit)

Use IAM terms precisely—this firmware **does not** implement transport-level **authentication** (proving which host or user agent sent a request) or fine-grained **authorization** (roles, scopes, policy engine).

| Question | What the tree does |
|------------|---------------------|
| **Who is the HTTP/WebUSB client?** | **Not verified.** No TLS client auth, no API keys, no `Authorization` header checks in `task_net.c` or the WebUSB path. Any host on the reachable LAN/USB may call the endpoints. |
| **Who may trigger signing?** | **Not bound to identity.** `POST /tx` enqueues a request if parsing/validation pass; the **device** then requires **physical button** confirmation before `task_sign` signs—this is **human approval**, not cryptographic proof of the HTTP client’s identity. |
| **Who may read `/tx/signed`?** | **Anyone** who can reach the device over HTTP—responses are not session-scoped or user-bound. |
| **UART CWUP** | **No** command authentication (documented elsewhere); lab/CI surface. |
| **Firmware trust** | **Different layer:** verified bootloader / image signing in `stm32_secure_boot` proves **code integrity**, not **user** or **peer** identity for wallet RPC. |

**Operator rule:** treat the network path as **unauthenticated**; rely on **network placement** (trusted LAN), **physical access** to approve on the device, and **OLED review**—not on wire secrecy or peer identity.

## 1.4 FreeRTOS shape

After `HW_Init()` and `osKernelStart()`, typical tasks include: display, net (LwIP + HTTP), sign, IO (LED/button), user input, optional USB device, optional CWUP task, optional RNG dump task.

**IPC** (queues, event groups, mutexes) connects network/WebUSB ingress to signing and display. Exact handles live in `wallet_shared.h` and `main.c`.

## 1.5 Build flags that change security posture

| Flag | Effect |
|------|--------|
| `USE_CRYPTO_SIGN` | Full trezor-crypto path vs minimal hash-only path |
| `USE_TEST_SEED` | **Known mnemonic** — development only, never for real funds |
| `USE_RNG_DUMP` | Binary TRNG on UART; **no** CWUP on that UART |
| `USE_WEBUSB` | USB device + WebUSB descriptors |
| `USE_LWIP` | Ethernet + HTTP server |

## 1.6 Relationship to `stm32_secure_boot`

- Bootloader signing and verification are documented there (e.g. `sign_image.py`, image layout).
- Host-side tests in this repo (`secure_boot_image.py`, `tests/mvp/…`) validate **app image** signing/tamper logic **as data**, not in-ROM execution.
- CWUP command `AT+BOOTCHAIN?` is **informational** (macros, `VTOR`); it does not prove hardware root of trust.
