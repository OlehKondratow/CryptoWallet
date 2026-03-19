# CryptoWallet

A microcontroller application for secure Bitcoin transaction signing on STM32H743. Integrates trezor-crypto, FreeRTOS, LwIP, SSD1306 UI, and WebUSB.

**Documentation available in:**
- 🇬🇧 [English](README.md) (this file)
- 🇷🇺 [Русский](README_ru.md)
- 🇵🇱 [Polski](README_pl.md)

---

## 🔐 Core & Security

Core signing logic, key management, validation, and cryptographic operations.

### [main.c](Core/Src/main.c) — Entry point and application management
FreeRTOS entry point: initialization of IPC objects (queues, semaphores, event groups), creation and startup of critical tasks.
**Full documentation:** [docs_src/main.md](docs_src/main.md)

### [task_sign.c](Core/Src/task_sign.c) — Signing pipeline (FSM)
The workhorse: validates request from queue, forms SHA-256, awaits user confirmation, ECDSA signature, saves result.
**Full documentation:** [docs_src/task_sign.md](docs_src/task_sign.md)

### [crypto_wallet.c](Core/Src/crypto_wallet.c) — Cryptographic layer (trezor-crypto)
Wrapper over trezor-crypto: STM32 RNG + entropy pooling, BIP-39 mnemonics, BIP-32 HD derivation (m/44'/0'/0'/0/0), ECDSA secp256k1.
**Full documentation:** [docs_src/crypto_wallet.md](docs_src/crypto_wallet.md)

### [tx_request_validate.c](Core/Src/tx_request_validate.c) — Validation gate
Guardian layer before signing: validates address (Base58/bech32), amount (decimal), currency (whitelist).
**Full documentation:** [docs_src/tx_request_validate.md](docs_src/tx_request_validate.md)

### [memzero.c](Core/Src/memzero.c) — Secure zeroing
Destructs sensitive buffers (keys, digests, seeds) via volatile writes, preventing compiler optimization.
**Full documentation:** [docs_src/memzero.md](docs_src/memzero.md)

### [sha256_minimal.c](Core/Src/sha256_minimal.c) — SHA-256 fallback
Compact SHA-256 implementation (when `USE_CRYPTO_SIGN=0` without trezor-crypto).
**Full documentation:** [docs_src/sha256_minimal.md](docs_src/sha256_minimal.md)

### [wallet_seed.c](Core/Src/wallet_seed.c) — Seed management (test)
Test seed for development (`USE_TEST_SEED=1`): BIP-39 vector "abandon...about", **development only**.
**Full documentation:** [docs_src/wallet_seed.md](docs_src/wallet_seed.md)

### [task_security.c](Core/Src/task_security.c) — Legacy signing (audit/test)
Alternative signing FSM with mock cryptography for bring-up and comparison.
**Full documentation:** [docs_src/task_security.md](docs_src/task_security.md)

**Headers:** crypto_wallet.h, memzero.h, sha256_minimal.h, task_sign.h, task_security.h, tx_request_validate.h, wallet_shared.h

---

## 📡 Communication Interfaces

Network stack (LwIP/Ethernet), USB (WebUSB), time synchronization.

### [task_net.c](Src/task_net.c) — HTTP server and network API
LwIP/Ethernet startup, HTTP on port 80, JSON/form `POST /tx` parsing, validation, enqueue to `g_tx_queue`.
**Full documentation:** [docs_src/task_net.md](docs_src/task_net.md)

### [usb_webusb.c](Core/Src/usb_webusb.c) — WebUSB vendor interface
Vendor-specific WebUSB: bulk endpoints, ping/pong, binary frame for signature request.
**Full documentation:** [docs_src/usb_webusb.md](docs_src/usb_webusb.md)

### [app_ethernet_cw.c](Src/app_ethernet_cw.c) — Ethernet link and DHCP FSM
Ethernet link callback for up/down, DHCP state machine (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback.
**Full documentation:** [docs_src/app_ethernet_cw.md](docs_src/app_ethernet_cw.md)

### [time_service.c](Core/Src/time_service.c) — SNTP and UTC
Time synchronization via SNTP, unified access to Unix epoch and UTC strings.
**Full documentation:** [docs_src/time_service.md](docs_src/time_service.md)

### [usb_device.c](Core/Src/usb_device.c) — USB device initialization
HSI48 clock configuration, USBD core initialization, WebUSB class registration.
**Full documentation:** [docs_src/usb_device.md](docs_src/usb_device.md)

### [usbd_conf_cw.c](Core/Src/usbd_conf_cw.c) — USB BSP configuration
Static allocator for USBD, MSP for PCD (GPIO AF, clock, NVIC), HAL_PCD bridging to USBD_LL.
**Full documentation:** [docs_src/usbd_conf_cw.md](docs_src/usbd_conf_cw.md)

### [usbd_desc_cw.c](Core/Src/usbd_desc_cw.c) — USB descriptors
Device/interface/BOS descriptors, strings (manufacturer, product, serial), WebUSB Platform Capability UUID.
**Full documentation:** [docs_src/usbd_desc_cw.md](docs_src/usbd_desc_cw.md)

**Headers:** task_net.h, usb_device.h, usb_webusb.h, usbd_conf.h, usbd_conf_cw.h, usbd_desc_cw.h, app_ethernet.h, time_service.h, lwipopts.h

---

## 🎨 User Experience

Display management, button handling, system events, and indicators.

### [task_display.c](Core/Src/task_display.c) — SSD1306 UI (full version)
Visual state management on SSD1306 128×32: 4 lines, scrolling log, state merging.
**Full documentation:** [docs_src/task_display.md](docs_src/task_display.md)

### [task_display_minimal.c](Core/Src/task_display_minimal.c) — SSD1306 UI (minimal)
Minimal UI for `minimal-lwip`: UART mirroring, periodic display updates.
**Full documentation:** [docs_src/task_display_minimal.md](docs_src/task_display_minimal.md)

### [task_user.c](Core/Src/task_user.c) — User button (PC13)
Physical UX: debouncing, short press (Confirm) vs long hold ~2.5s (Reject).
**Full documentation:** [docs_src/task_user.md](docs_src/task_user.md)

### [task_io.c](Core/Src/task_io.c) — LED indicators
Visual indicators: LED1 = alive heartbeat, LED2 = network activity, LED3 = security alert.
**Full documentation:** [docs_src/task_io.md](docs_src/task_io.md)

**Headers:** task_display.h, task_user.h, task_io.h

---

## ⚙️ System & Hardware

HAL initialization, clocking, interrupt handlers, driver configuration.

### [hw_init.c](Core/Src/hw_init.c) — Board bring-up (clocks, MPU, GPIO, I2C, UART)
Low-level bootstrap: clock configuration, MPU/cache (for LwIP), GPIO (LED, button), I2C1 (OLED), UART, USB, RNG.
**Full documentation:** [docs_src/hw_init.md](docs_src/hw_init.md)

### [stm32h7xx_hal_msp.c](Core/Src/stm32h7xx_hal_msp.c) — HAL MSP callbacks
Hardware-level configuration: `HAL_I2C_MspInit` (I2C1), `HAL_UART_MspInit` (USART3).
**Full documentation:** [docs_src/stm32h7xx_hal_msp.md](docs_src/stm32h7xx_hal_msp.md)

### [stm32h7xx_it.c](Core/Src/stm32h7xx_it.c) — Interrupt handlers (main)
Handlers: `SysTick_Handler` (FreeRTOS tick), Ethernet IRQ (when `USE_LWIP`).
**Full documentation:** [docs_src/stm32h7xx_it.md](docs_src/stm32h7xx_it.md)

### [stm32h7xx_it_systick.c](Core/Src/stm32h7xx_it_systick.c) — SysTick for minimal-lwip
Alternative `SysTick_Handler` for `minimal-lwip` build.
**Full documentation:** [docs_src/stm32h7xx_it_systick.md](docs_src/stm32h7xx_it_systick.md)

### [stm32h7xx_it_usb.c](Core/Src/stm32h7xx_it_usb.c) — USB OTG HS IRQ
OTG HS interrupt handler: calls `HAL_PCD_IRQHandler` for WebUSB.
**Full documentation:** [docs_src/stm32h7xx_it_usb.md](docs_src/stm32h7xx_it_usb.md)

### [ssd1306_conf.h](Drivers/ssd1306/ssd1306_conf.h) — Display driver configuration
Build-time parameters: I2C1 binding, address 0x3C, geometry 128×32, font 6×8.
**Full documentation:** [docs_src/ssd1306_conf.md](docs_src/ssd1306_conf.md)

**Headers:** hw_init.h, main.h, lwipopts.h

---

## 📚 Documentation and Reference

- **[docs_src/README.md](docs_src/README.md)** — Complete index of all 32 modules with hierarchical navigation
- **[docs_src/doxygen-comments.md](docs_src/doxygen-comments.md)** — Doxygen comment style guidelines
- **[docs_src/api-documentation-scope.md](docs_src/api-documentation-scope.md)** — Documentation progress tracking

---

## 🚀 Quick Start

### Documentation Structure
1. **Code (.c/.h)** — minimal @brief/@details, API level
2. **docs_src/*.md** — detailed logic explanations (Abstract → Logic Flow → Dependencies)
3. **Doxygen HTML** — code cross-references (`make docs-doxygen`)

### How to Read a Module
1. Open [docs_src/README.md](docs_src/README.md)
2. Find your module of interest
3. Start with **Abstract** (business logic) → **Logic Flow** (algorithm) → **Dependencies**
4. Follow **Relations** for broader context

### Main Commands
```bash
make docs-doxygen    # Generate Doxygen
make build          # Build
make minimal-lwip   # Minimal build
make flash          # Flash to STM32
```

---

## 📋 Complete Module Index

<!-- DOXYGEN_DOCS_SRC_INDEX -->
| Module | Brief Overview |
|--------|------------------|
| [api-documentation-scope](docs_src/api-documentation-scope.md) | Tracking documentation progress and Doxygen coverage for all modules. |
| [app_ethernet](docs_src/app_ethernet.md) | Header file defining Ethernet "glue" layer interfaces: link callback, DHCP FSM constants. |
| [app_ethernet_cw](docs_src/app_ethernet_cw.md) | Ethernet support: link-up/down FSM, DHCP state machine (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback. |
| [crypto_wallet](docs_src/crypto_wallet.md) | Wraps trezor-crypto library: STM32 RNG with entropy pooling, BIP-39, BIP-32 HD derivation, ECDSA secp256k1 signing. |
| [doxygen-comments](docs_src/doxygen-comments.md) | Guidelines for Doxygen comment style: @brief/@details separation in code. |
| [hw_init](docs_src/hw_init.md) | Board bring-up: clock configuration, MPU/cache (for LwIP), GPIO, I2C1, UART, USB, RNG initialization. |
| [lwipopts](docs_src/lwipopts.md) | LwIP compile-time configuration: IPv4/TCP/DHCP/DNS/SNTP, heap parameters, TCP buffers/window. |
| [main](docs_src/main.md) | FreeRTOS entry point and application orchestration. |
| [memzero](docs_src/memzero.md) | Secure buffer zeroing using volatile writes to prevent compiler optimization. |
| [README](docs_src/README.md) | Complete index of 32 modules with hierarchical navigation and documentation methodology. |
| [sha256_minimal](docs_src/sha256_minimal.md) | Compact SHA-256 implementation for minimal builds (USE_CRYPTO_SIGN=0). |
| [ssd1306_conf](docs_src/ssd1306_conf.md) | Display driver build-time configuration. |
| [stm32h7xx_hal_msp](docs_src/stm32h7xx_hal_msp.md) | HAL MSP callbacks for I2C1 and USART3 hardware configuration. |
| [stm32h7xx_it](docs_src/stm32h7xx_it.md) | Interrupt handlers: SysTick (FreeRTOS tick), Ethernet IRQ. |
| [stm32h7xx_it_systick](docs_src/stm32h7xx_it_systick.md) | Alternative SysTick handler for minimal-lwip build. |
| [stm32h7xx_it_usb](docs_src/stm32h7xx_it_usb.md) | USB OTG HS interrupt handler for WebUSB. |
| [task_display](docs_src/task_display.md) | SSD1306 UI management: 4-line display, scrolling log, state merging. |
| [task_display_minimal](docs_src/task_display_minimal.md) | Minimal UI for minimal-lwip: UART mirroring, reduced SSD1306 load. |
| [task_io](docs_src/task_io.md) | LED indicator task: alive heartbeat, network activity, security alerts. |
| [task_net](docs_src/task_net.md) | HTTP server and network API: LwIP/Ethernet, port 80, POST /tx parsing and validation. |
| [task_security](docs_src/task_security.md) | Legacy signing FSM with mock cryptography (test/audit only). |
| [task_sign](docs_src/task_sign.md) | Main signing pipeline: queue consumer, SHA-256 forming, user confirmation, ECDSA signature. |
| [task_user](docs_src/task_user.md) | User button handling: debouncing, short/long press distinction, confirm/reject signaling. |
| [time_service](docs_src/time_service.md) | SNTP time synchronization and UTC string formatting. |
| [tx_request_validate](docs_src/tx_request_validate.md) | Input validation gate: address (Base58/bech32), amount (decimal), currency (whitelist) verification. |
| [usb_device](docs_src/usb_device.md) | USB device initialization: HSI48 clock, USBD core, WebUSB class registration. |
| [usb_webusb](docs_src/usb_webusb.md) | WebUSB vendor interface: bulk endpoints, ping/pong, binary signature request/response. |
| [usbd_conf](docs_src/usbd_conf.md) | USB device configuration wrapper. |
| [usbd_conf_cw](docs_src/usbd_conf_cw.md) | USB BSP configuration for CryptoWallet WebUSB. |
| [usbd_desc_cw](docs_src/usbd_desc_cw.md) | USB device descriptors for WebUSB. |
| [wallet_seed](docs_src/wallet_seed.md) | Test seed stub (USE_TEST_SEED=1, development only). |
| [wallet_shared](docs_src/wallet_shared.md) | Shared IPC types, structures, and global object contracts. |
<!-- /DOXYGEN_DOCS_SRC_INDEX -->
