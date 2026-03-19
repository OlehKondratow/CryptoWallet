# Code reference (auto-generated)

!!! warning "Generated file"
    Do not edit by hand. Regenerate:

    ```bash
    make docs-code-md
    # or: python3 scripts/generate_code_reference_md.py
    ```

**Generated:** 2026-03-19 22:46 UTC  
**Source:** file headers (`@file` / `@brief` / `@details` in `/** ... */`) and Python module docstrings in `scripts/*.py`.  
**Design:** [code-doc-generation](code-doc-generation.md)

## Firmware — Core/Src

### `Core/Src/crypto_wallet.c`
**@brief:** trezor-crypto glue: STM32 TRNG, @c random_buffer , BIP-32, ECDSA sign.

**@details**

**RNG:** @c random_buffer() mixes HAL RNG + entropy pool; used by trezor-crypto.
**When @c USE_CRYPTO_SIGN=0 :** hashing helpers fall back to @c sha256_minimal.c .
**Makefile** links selected @c ThirdParty/trezor-crypto objects (see @c Makefile ).
**Entropy / testing:** @c docs_src/rng-entropy.md , @c scripts/capture_rng_uart.py ,
@c scripts/run_dieharder.py .
**Integration overview:** @c docs_src/trezor-crypto-integration.md .

### `Core/Src/hw_init.c`
**@brief:** Board bring-up: clock, MPU/cache, GPIO, I2C1 (OLED), UART, optional USB.

### `Core/Src/main.c`
**@brief:** FreeRTOS entry: IPC objects, task creation, OS hooks.

**@details**

**Product (high-level):** Embedded wallet on STM32H743 (Nucleo-144): receive
sign requests over Ethernet (HTTP) and/or WebUSB, confirm on USER button,
show status on SSD1306; optional full ECDSA via trezor-crypto (@c USE_CRYPTO_SIGN ).
**Tasks started here:** @c Task_Display_Create , @c Task_Net_Create ,
@c Task_Sign_Create , @c Task_IO_Create , @c Task_User_Create .
@c task_security.c is linked, but @c Task_Security_Create() is **not** called
from this file — production signing runs in @c task_sign.c .
**IPC (globals by design):** @c g_tx_queue (net → sign), @c g_display_queue ,
@c g_user_event_group (user button → sign), I2C/UI/display mutexes — see
@c wallet_shared.h .
**Boot order:** with @c USE_LWIP , @c HW_Init_Early_LwIP() before @c HAL_Init()
(MPU/cache, lwip_zero order). Then @c HW_Init() , @c time_service_init() ,
@c crypto_rng_init() when crypto enabled.
**Build flags:** @c BOOT_TEST — no FreeRTOS; @c SKIP_OLED — skip OLED path.
**Hooks:** @c vApplicationMallocFailedHook , @c vApplicationStackOverflowHook ,
@c Error_Handler .
See @c docs_src/architecture.md , repository @c README.md .

### `Core/Src/memzero.c`
**@brief:** Secure @c memzero() — volatile byte writes (no optimize-out).

**@details**

**Role:** Clear seeds, digests, session buffers after use (signing stack,
@c task_security.c , @c wallet_seed.c , crypto paths). Same pattern as
common crypto libraries on bare metal.

### `Core/Src/sha256_minimal.c`
**@brief:** SHA-256 only — used when USE_CRYPTO_SIGN=0 (no trezor-crypto).

**@details**

Public-domain compact implementation. Signing path with full crypto uses
trezor-crypto sha2.c instead. **Headers:** sha256_minimal.h .

### `Core/Src/stm32h7xx_hal_msp.c`
**@brief:** MSP init for CryptoWallet - I2C1 (SSD1306).

### `Core/Src/stm32h7xx_it.c`
**@brief:** Interrupt handlers - FreeRTOS SysTick, ETH (when USE_LWIP).

### `Core/Src/stm32h7xx_it_systick.c`
**@brief:** SysTick handler for minimal-lwip (FreeRTOS tick).

**@details**

LWIP_APP stm32h7xx_it.c lacks SysTick_Handler; without it,
SysTick goes to Default_Handler → infinite loop / "WWDG" backtrace.

### `Core/Src/stm32h7xx_it_usb.c`
**@brief:** USB OTG HS interrupt handler (WebUSB).

### `Core/Src/task_display.c`
**@brief:** SSD1306 128×32 — four scroll lines, state machine, queue-driven UI.

**@details**

**Inputs:** @c g_display_queue (@c Transaction_Data_t from net),
@c g_display_ctx + @c g_display_ctx_mutex (signing/net status),
@c Task_Display_Log() — **UART + on-screen log** (architecture: logging hub).
**UI states:** @c UI_State_t / @c display_state_t — WALLET, SECURITY,
NETWORK, LOG (amount, lock/sig, IP/MAC, scroll buffer).
**Concurrency:** @c g_i2c_mutex wraps I2C; @c g_ui_mutex for merged UI data.
**Clean-code note:** @c display_task / @c render_four_scroll_lines are long;
acceptable for embedded; could be split further.
**Docs:** @c docs_src/architecture.md , @c README.md .

### `Core/Src/task_display_minimal.c`
**@brief:** Reduced display task for `minimal-lwip` — faster Ethernet-first bring-up.

**@details**

**Purpose:** Same logging hook as full UI but minimal OLED/state machine load
while validating LwIP. Swapped in by Makefile instead of @c task_display.c .
@c SKIP_OLED=1 skips I2C if bus hangs. **Production UI:** @c task_display.c .

### `Core/Src/task_io.c`
**@brief:** LED policy task — system / network / alert indicators only.

**@details**

**Scope:** LED1 green (alive), LED2 yellow (network hint), LED3 red
(@c g_security_alert ). **Does not** read USER button — that is
@c task_user.c → @c g_user_event_group → @c task_sign.c .
Matches “one module, one responsibility” from architecture review.
**Pins:** @c main.h .

### `Core/Src/task_security.c`
**@brief:** Alternate signing FSM with **mock** SHA256/ECDSA (placeholders).

**@details**

**Status:** Object file is linked, but @c Task_Security_Create() is **not**
invoked from @c main.c . Production firmware uses @c task_sign.c with
real crypto when @c USE_CRYPTO_SIGN . Keep this module for bring-up or
comparison with Trezor-style FSM (IDLE→RECEIVED→WAIT_CONFIRM→…).
Uses @c memzero() for sensitive buffers; confirm timeout 30 s.
**Docs:** @c docs_src/architecture.md (task table + data-flow).

### `Core/Src/task_sign.c`
**@brief:** Primary signing task — consumes @c g_tx_queue , USER confirm, ECDSA.

**@details**

**Architecture position:** Main consumer of @c g_tx_queue (from @c task_net
and potentially WebUSB path). Producer of @c g_last_sig and display updates.
This is the **active** signing path in @c main.c (@c Task_Sign_Create ).
**Pipeline:** Dequeue @c wallet_tx_t → @c tx_request_validate → build
deterministic string → SHA-256 → wait @c EVENT_USER_CONFIRMED or reject.
Derive @c m/44'/0'/0'/0/0 , @c crypto_sign_btc_hash() → @c g_last_sig .
**Seed:** @c get_wallet_seed() (weak stub or @c wallet_seed.c with @c USE_TEST_SEED ).
**Crypto:** @c crypto_wallet.c / trezor-crypto when @c USE_CRYPTO_SIGN .
Original wallet logic; no code copied from trezor-firmware.
**Docs:** @c docs_src/verification-signing.md , @c docs_src/crypto-messages.md .

### `Core/Src/task_user.c`
**@brief:** Physical UX — USER (PC13) debounce, confirm vs reject for signing.

**@details**

**Architecture:** Sole owner of the USER GPIO for wallet policy; isolates
button timing from @c task_io.c (LEDs only). Short press →
@c EVENT_USER_CONFIRMED ; ~2.5 s hold → @c EVENT_USER_REJECTED .
Debounce 50 ms; @c task_sign.c waits on @c g_user_event_group .
**Docs:** @c docs_src/testing-signing.md , @c docs_src/architecture.md .

### `Core/Src/time_service.c`
**@brief:** SNTP client — wall-clock epoch and UTC strings for logs/UI.

**@details**

**Support module** in architecture: not on the signing hot path; provides
time after Ethernet is up (@c time_service_start() ). Ported from lwip_zero,
uses @c pool.ntp.org , logs via @c Task_Display_Log() (UART + OLED line).
Leap-year and epoch→UTC formatting for human-readable timestamps.

### `Core/Src/tx_request_validate.c`
**@brief:** Validate host-supplied recipient / amount / currency before signing.

**@details**

Whitelist currencies, Base58 checks for BTC-style addresses, numeric
amount rules. Used from task_net.c (HTTP), usb_webusb.c (WebUSB), and
task_sign.c. **Messages:** docs_src/crypto-messages.md .

### `Core/Src/usb_device.c`
**@brief:** USB device initialization for CryptoWallet WebUSB.

### `Core/Src/usb_webusb.c`
**@brief:** WebUSB vendor class — ping/pong and binary sign request/response.

**@details**

**Descriptors / IDs:** @c Core/Src/usbd_desc_cw.c (e.g. VID/PID for host).
**Framing:** command byte + payload; responses include status + 64-byte
ECDSA compact sig when applicable.
**Host tools:** @c scripts/test_usb_sign.py (pyusb). **Docs:** @c docs_src/webusb.md ,
@c udev/99-cryptowallet-webusb.rules on Linux.
Guarded by @c USE_WEBUSB .

### `Core/Src/usbd_conf_cw.c`
**@brief:** USB device BSP for CryptoWallet WebUSB (NUCLEO-H743ZI2, PA11/PA12).

### `Core/Src/usbd_desc_cw.c`
**@brief:** USB device descriptors for CryptoWallet WebUSB.

### `Core/Src/wallet_seed.c`
**@brief:** Strong get_wallet_seed() when USE_TEST_SEED=1 (development only).

**@details**

BIP-39 vector "abandon ... about" → 64-byte seed. **Never for real funds.**
Production: replace with secure element / flash encryption workflow.
**Audit:** scripts/bootloader_secure_signing_test.py --elf-audit-only .
**Build:** Makefile USE_TEST_SEED=1 (implies USE_CRYPTO_SIGN).

## Firmware — Core/Inc

### `Core/Inc/app_ethernet.h`
**@brief:** Ethernet link callback and DHCP thread interface.

**@details**

Implemented in app_ethernet_cw.c. Used by ethernetif and task_net.

### `Core/Inc/crypto_wallet.h`
**@brief:** trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1.

**@details**

Requires trezor-crypto (github.com/trezor/trezor-crypto).
Target: STM32H743 (TRNG + timer entropy). ARM GCC compatible.

### `Core/Inc/hw_init.h`
**@brief:** Hardware initialization wrapper (CMSIS-compliant).

### `Core/Inc/main.h`
**@brief:** Board pins, LEDs, UART logging macro, network IP defaults.

**@details**

STM32H743ZI2 (Nucleo-144). **Documentation index:** repository @c README.md .
**Pinout table:** @c Core/Src/hw_init.c and @c docs_src/pinout.md .
Adjust @c IP_ADDR0..3 , netmask, gateway for your LAN; DHCP vs static in
@c Core/Inc/lwipopts.h .

### `Core/Inc/memzero.h`
**@brief:** Secure memory zeroing (prevents compiler optimization).

**@details**

Use for clearing sensitive data (keys, digest, signatures).
Pattern from Trezor/crypto and libsodium.

### `Core/Inc/sha256_minimal.h`
**@brief:** Public API for minimal SHA-256 when trezor-crypto is disabled.

**@details**

Implemented in @c sha256_minimal.c . Output is big-endian digest (32 bytes).

### `Core/Inc/task_display.h`
**@brief:** OLED task API — SSD1306 UI types and @c Task_Display_Log .

**@details**

**Role:** Defines how @c task_net , @c task_sign , and others push UI data:
@c Transaction_Data_t on @c g_display_queue ; @c Task_Display_Log() writes
UART + scrollable log line (see @c docs_src/architecture.md ).
**USER confirm** is not in this module — @c task_user.c sets event bits
consumed by @c task_sign.c .

### `Core/Inc/task_io.h`
**@brief:** IO module - LEDs only.

**@details**

LED1 (Green) System OK, LED2 (Yellow) Network, LED3 (Red) Security.
USER button handling in task_user.c.

### `Core/Inc/task_net.h`
**@brief:** LwIP + HTTP — Ethernet stack and port 80 API for transactions.

**@details**

Parses @c POST /tx (JSON or form), validates, enqueues @c wallet_tx_t to
@c g_tx_queue for @c task_sign.c . Pushes display updates via
@c g_display_queue . May call @c Task_Display_Log() (direct log — common
in embedded). Without @c USE_LWIP , @c Task_Net_Create() is empty.

### `Core/Inc/task_security.h`
**@brief:** Header for legacy @c task_security.c (mock crypto FSM).

**@details**

**Not started from @c main.c** — see @c task_sign.c for the queue consumer
fed by @c task_net . USER events come from @c task_user.c via
@c g_user_event_group . Mock SHA256/ECDSA here vs real trezor path in sign task.

### `Core/Inc/task_sign.h`
**@brief:** Production signing task — @c g_tx_queue consumer, USER confirm, ECDSA.

**@details**

**Active path in @c main.c** (@c Task_Sign_Create ). Pulls @c wallet_tx_t ,
validates via @c tx_request_validate , hashes, waits on
@c g_user_event_group , signs with @c crypto_wallet / trezor-crypto.
Original wallet logic; not copied from trezor-firmware.

### `Core/Inc/task_user.h`
**@brief:** User button task — USER key (PC13) handling.

**@details**

Dedicated task for USER button. Debounce 50ms. Short press = Confirm,
long press (2.5s) = Reject. Signals task_sign via g_user_event_group.

### `Core/Inc/time_service.h`
**@brief:** SNTP API — init, start after link, epoch + formatted UTC.

**@details**

Call @c time_service_init() before @c tcpip_init ; @c time_service_start()
after Ethernet link up. Feeds wallet logs / UI time strings; see
@c time_service.c and @c docs_src/architecture.md (support modules).

### `Core/Inc/tx_request_validate.h`
**@brief:** Request analysis and validation for crypto transaction signing.

**@details**

Original implementation — not copied from trezor-firmware. All host-supplied
strings are checked before they reach @c g_tx_queue or signing. Reduces risk of
malformed UI and obviously invalid addresses. **Not** a substitute for full
on-chain transaction validation.

### `Core/Inc/usb_device.h`
**@brief:** USB device init for CryptoWallet WebUSB.

### `Core/Inc/usb_webusb.h`
**@brief:** WebUSB vendor-specific class for CryptoWallet.

**@details**

Vendor-specific interface (0xFF) with bulk endpoints.
Compatible with Chrome WebUSB API.

### `Core/Inc/usbd_conf.h`
**@brief:** USB device conf - redirects to CryptoWallet WebUSB config.

### `Core/Inc/usbd_conf_cw.h`
**@brief:** USB device BSP configuration for CryptoWallet WebUSB.

### `Core/Inc/usbd_desc_cw.h`
**@brief:** USB device descriptors for CryptoWallet WebUSB.

### `Core/Inc/wallet_shared.h`
**@brief:** Shared types and IPC: queues, events, mutexes, display context.

**@details**

**Data flow (architecture overview):**
- @c task_net : HTTP @c POST /tx → validate → @c g_tx_queue → @c task_sign .
- @c task_net : UI hints → @c g_display_queue → @c task_display .
- @c task_user : USER (PC13) → @c g_user_event_group → @c task_sign .
- @c task_sign : status/result → @c g_display_ctx → @c task_display .
- @c task_display : SSD1306 I2C under @c g_i2c_mutex .
**Queues:** @c g_tx_queue (@c wallet_tx_t ); @c g_display_queue
(@c Transaction_Data_t in @c task_display.h ).
**Events:** @c EVENT_USER_CONFIRMED (short press),
@c EVENT_USER_REJECTED (long ~2.5 s).
**Mutexes:** @c g_i2c_mutex ; @c g_display_ctx_mutex ; @c g_ui_mutex
(display task internal UI merge).
**Display states ( @c display_state_t ):** WALLET, SECURITY, NETWORK, LOG.
**Signing FSM ( @c signing_state_t )** — used by @c task_sign.c (and
@c task_security.c if enabled).
**Output signature:** @c g_last_sig[64] , @c g_last_sig_ready .
Diagram + coding notes: @c docs_src/architecture.md .

## Firmware — Src

### `Src/app_ethernet_cw.c`
**@brief:** Ethernet glue — link callbacks, DHCP state machine, LED feedback.

**@details**

**Support module:** Bridges LwIP/netif to board LEDs and user feedback.
@c ethernet_link_status_updated() on link change; DHCP thread when
@c LWIP_DHCP . LED2 = network activity hint, LED3 = link-down warning.
Logs through @c Task_Display_Log() . Starts @c time_service_start() when
appropriate. Ported from lwip_zero.

### `Src/task_net.c`
**@brief:** LwIP + HTTP — DHCP/static IP, POST /tx, signing poll endpoints.

**@details**

**Functional role:** Edge of the wallet facing the LAN — TCP/IP, HTTP
server, JSON/form parsing **without** an external JSON library (size).
**HTTP:** Port 80. Body JSON or form: recipient, amount, currency.
Validated via @c tx_request_validate.c , then @c wallet_tx_t to
@c g_tx_queue → @c task_sign.c (not @c task_security.c ).
Ping / signed-result endpoints — @c docs_src/api.md .
**IP:** DHCP with static fallback (e.g. 192.168.0.10) in @c main.h .
**Build:** @c USE_LWIP ; BSP from Cube / @c stm32_secure_boot template paths.
**Tests:** @c docs_src/testing-signing.md , @c scripts/bootloader_secure_signing_test.py .

## Drivers

### `Drivers/ssd1306/ssd1306_conf.h`
**@brief:** Display driver tuning — I2C1, 128×32, 0x3C, Font 6×8.

**@details**

Binds Cube @c hi2c1 to the **ssd1306** sources from sibling repo
@c stm32-ssd1306 . Consumed by @c task_display.c / @c task_display_minimal.c .
Set @c SSD1306_USE_ADDR_3D to 1 if the module uses 0x3D (SA0 high).

## Scripts

### `scripts/bootloader_secure_signing_test.py`
```text
Boot + secure signing smoke test for CryptoWallet (NUCLEO-H743ZI2).

Documentation index: repository README.md §2 (scripts) and §1 (firmware modules).

Context
-------
CryptoWallet is usually linked as a **single application** at flash base **0x08000000**
(linker/RTOS pieces come from sibling repo ``stm32_secure_boot``). This script does **not**
implement a custom MCU bootloader; it automates:

1. **Build** — ``minimal-lwip`` with signing + test seed (and optional WebUSB).
2. **Flash** — ``st-flash`` to 0x08000000 (same as ``make flash-minimal-lwip``).
3. **Boot / reachability** — HTTP ``GET /ping`` or UART line wait (optional).
4. **Signing** — ``POST /tx`` → you confirm on USER (PC13) → ``GET /tx/signed``.
5. **Protected-build sanity** — optional scan of ``.elf`` for obvious test-secret strings.

For a **separate** secure-bootloader project (verified boot, RDP, option bytes), use
``../stm32_secure_boot`` and its docs/scripts; flash this app only after your boot policy
allows it.

Dependencies: Python 3.8+ (stdlib). Optional: ``pyserial`` for ``--uart-wait``.

Usage
-----
  # Full flow (Ethernet + HTTP); tries common static IPs if --ip omitted
  python3 scripts/bootloader_secure_signing_test.py

  # Already built & flashed; device at fixed IP
  python3 scripts/bootloader_secure_signing_test.py --no-build --no-flash --ip 192.168.0.10

  # WebUSB signing only (still builds with USE_WEBUSB=1 unless --no-build)
  python3 scripts/bootloader_secure_signing_test.py --webusb --no-flash

  # Only check .elf for test mnemonic leakage (after build)
  python3 scripts/bootloader_secure_signing_test.py --elf-audit-only --no-flash --no-sign
```
### `scripts/capture_rng_uart.py`
```text
Capture raw random bytes from the device UART into a binary file for DIEHARDER.

Doc index: README.md (repo root). RNG background: docs_src/rng-entropy.md .

Prerequisites on the device
---------------------------
Firmware must stream **only raw bytes** (no UART_Log / printf text). Typical options:

1. Add a diagnostic mode (e.g. USE_RNG_DUMP=1) that calls random_buffer() / HAL_RNG
   in a loop and writes bytes to the same UART used for logs — **without** any text.
2. Or use a dedicated USB CDC endpoint that sends binary only.

If ASCII logs are mixed into the stream, statistical tests are invalid.

Usage (from project root)
-------------------------
  pip install -r scripts/requirements.txt
  python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728

Recommended file size for dieharder -a: >= 128 MiB (134217728 bytes); more is better.
```
### `scripts/generate_code_reference_md.py`
```text
Generate docs_src/reference-code.md from file-level Doxygen headers and Python docstrings.

See docs_src/code-doc-generation.md for design and alternatives.
```
### `scripts/generate_readme_languages.py`
```text
Generate README in multiple languages (EN, RU, PL) from base English README.
Translates section headers and descriptive text while preserving links and code.

Usage:
    python3 scripts/generate_readme_languages.py
    
Output:
    - README.md (English - unchanged)
    - README_ru.md (Russian)
    - README_pl.md (Polish)
```
### `scripts/run_dieharder.py`
```text
Run dieharder on a binary file of raw random bytes (generator 201 = file_input_raw).

Doc index: README.md (repo root). Workflow: docs_src/rng-entropy.md , scripts/capture_rng_uart.py .

Install (Debian/Ubuntu):
  sudo apt install dieharder

Usage (from project root)
-------------------------
  python3 scripts/run_dieharder.py --file rng.bin
  python3 scripts/run_dieharder.py --file rng.bin --list-tests
  python3 scripts/run_dieharder.py --file rng.bin --test 1 --subtest 0
```
### `scripts/test_plan_signing_rng.py`
```text
Test plan: Bitcoin transaction signing + RNG (including DIEHARDER file workflow).

Documentation index: repository README.md (maps this script to firmware + docs).

Print markdown to stdout or write to a file (run from project root):

  python3 scripts/test_plan_signing_rng.py
  python3 scripts/test_plan_signing_rng.py --write docs_src/testing-plan-signing-rng.md

``make docs`` invokes this script before MkDocs build.
```
### `scripts/test_usb_sign.py`
```text
Test script for transaction signing over WebUSB.

Documentation index: README.md (repo root). Protocol notes: Core/Src/usb_webusb.c , docs_src/webusb.md .

Requirements:
  pip install -r scripts/requirements.txt   # pyusb (+ pyserial for other scripts)
  Linux: libusb-1.0 (apt install libusb-1.0-0-dev), udev rules
         udev/99-cryptowallet-webusb.rules → /etc/udev/rules.d/

Usage:
  python3 scripts/test_usb_sign.py ping          # connectivity (ping → pong)
  python3 scripts/test_usb_sign.py sign          # signing request (confirm USER on device)
  python3 scripts/test_usb_sign.py sign --recipient 1A1zP1... --amount 0.001 --currency BTC
```
### `scripts/translate_docs.py`
```text
Create multi-language documentation copies: EN, RU, PL.
Uses simple pattern-based translation + Google Translate API fallback.
```
### `scripts/update_docs_src_index.py`
```text
Generate README index from hand-written educational docs in `docs_src/`.

Rules:
1) Scan `docs_src/*.md`
2) Extract short text from `## Краткий обзор` (or `## Summary`)
3) Create a Markdown table with links to each full doc
4) Inject the table into root `README.md` between markers
```
### `scripts/update_readme.py`
```text
Update README.md "Project Structure" from Doxygen XML and/or emit Markdown per file.

Doxygen does not output Markdown natively (no GENERATE_MARKDOWN). This script:
1. Parses docs_doxygen/xml/ (run: doxygen Doxyfile).
2. Extracts @brief (short) and @details (detailed) for each file/compound.
3. Updates root README.md: replaces the table in "## Project Structure" with generated rows.
4. Optionally writes docs_doxygen/md/<file>.md with brief + details (Markdown output).

Usage:
  doxygen Doxyfile
  python3 scripts/update_readme.py
  python3 scripts/update_readme.py --readme README.md --xml docs_doxygen/xml
  python3 scripts/update_readme.py --md-dir docs_doxygen/md   # emit .md per source file
```
