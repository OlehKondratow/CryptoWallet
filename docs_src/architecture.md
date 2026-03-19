\page architecture "CryptoWallet: Architecture and System Design"

# CryptoWallet Architecture Overview

## Project Summary

**CryptoWallet** is a modular Bitcoin transaction signing firmware for the **STM32H743ZI2** microcontroller (Nucleo-H743ZI2) running **FreeRTOS**. It integrates cryptographic operations (BIP-39, BIP-32, ECDSA secp256k1), hardware security (MPU, TRNG), networking (LwIP, Ethernet, WebUSB), and user-facing interfaces (OLED display, button I/O).

## High-Level Architecture

The system follows a **task-based, event-driven architecture**:

```
┌─────────────────────────────────────────────────────────┐
│  STM32H743ZI2 (ARM Cortex-M7, 480 MHz, 2 MB Flash)    │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────┐   │
│  │  FreeRTOS Scheduler (CMSIS-RTOS2)               │   │
│  ├──────────────────────────────────────────────────┤   │
│  │ Task 1: hw_init         (Initialization)        │   │
│  │ Task 2: task_display    (OLED UI, SSD1306)      │   │
│  │ Task 3: task_io         (LED/Button control)    │   │
│  │ Task 4: task_net        (Networking, LwIP)      │   │
│  │ Task 5: task_sign       (Cryptographic signing) │   │
│  │ Task 6: task_security   (Auth/validation)       │   │
│  │ Task 7: usb_device      (WebUSB, CDC)           │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Queues, Event Groups, Mutexes (IPC)           │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  HAL / Drivers (GPIO, I2C, UART, Eth, USB)     │   │
│  ├──────────────────────────────────────────────────┤   │
│  │  LwIP TCP/IP Stack  │  trezor-crypto            │   │
│  │  (DHCP, SNTP, DNS)  │  (BIP39/32, ECDSA, SHA)   │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
     │         │         │         │         │
     ▼         ▼         ▼         ▼         ▼
  ┌─────┐ ┌─────┐ ┌──────┐ ┌────────┐ ┌────────┐
  │OLED │ │LED/ │ │Ether-│ │ WebUSB │ │ UART   │
  │(I2C)│ │BTN  │ │ net  │ │(USB-HS)│ │(Debug) │
  └─────┘ └─────┘ └──────┘ └────────┘ └────────┘
```

## Core Components

### 1. Initialization (`main.c` / `hw_init.c`)

**Responsibilities:**
- MPU (Memory Protection Unit) configuration **before** HAL_Init (required for LwIP DMA safety)
- Cache enable/disable based on memory regions
- Clock tree initialization (PLL, clock distribution)
- NVIC setup for interrupt priorities
- RNG (TRNG) seeding for cryptographic entropy
- Peripheral clock gating and GPIO configuration
- FreeRTOS scheduler startup

**Key Flags:**
- `USE_LWIP` — Enable Ethernet + LwIP stack
- `USE_WEBUSB` — Enable USB WebUSB mode
- `USE_CRYPTO_SIGN` — Enable full ECDSA/BIP-39/BIP-32 (vs. SHA-256 only)
- `USE_TEST_SEED` — Use hardcoded test seed (development only)
- `BOOT_TEST` — Diagnostic mode without FreeRTOS (uart debug loop)

### 2. Inter-Task Communication (IPC)

**Global Synchronization Objects:**

```c
// Queues
QueueHandle_t g_tx_queue;          // task_net → task_sign (wallet_tx_t)
QueueHandle_t g_display_queue;     // task_net → task_display (Transaction_Data_t)

// Event Groups
EventGroupHandle_t g_user_event_group; // task_io → task_sign (user confirmation)

// Mutexes
SemaphoreHandle_t g_i2c_mutex;         // I2C1 bus arbitration
SemaphoreHandle_t g_display_ctx_mutex; // Display context protection
```

**Data Flow:**
1. Network packet arrives (HTTP POST `/tx` or WebUSB endpoint)
2. Parsed in `task_net` → posted to `g_tx_queue`
3. `task_sign` receives, displays transaction on OLED
4. User presses button → sets event in `g_user_event_group`
5. `task_sign` derives key, signs transaction, sends response

### 3. Display Task (`task_display.c`)

**Finite State Machine (FSM):**

| State | Display Mode | Purpose |
|---|---|---|
| `DISPLAY_WALLET` | Crypto ticker + amount | Show transaction details |
| `DISPLAY_SECURITY` | Lock status + signature | Auth/signing confirmation |
| `DISPLAY_NETWORK` | IP/MAC + USB status | Network connectivity info |
| `DISPLAY_LOG` | Scrollable system log | Debug/diagnostics |

**Hardware:** SSD1306 OLED (128×32, I2C1, addr 0x3C, PB8/PB9)

### 4. Cryptographic Integration (`crypto_wallet.c` / `task_sign.c`)

**Signing Pipeline:**

```
User Input (HTTP POST /tx or WebUSB)
    ↓
Validation (address format, amount, currency whitelist)
    ↓
build_hash_input() → "recipient|amount|currency"
    ↓
crypto_hash_sha256() → 32-byte digest
    ↓
User Confirmation (button press on OLED)
    ↓
get_wallet_seed() → 64-byte BIP-39 seed
    ↓
crypto_derive_btc_m44_0_0_0_0() → private key (32 bytes)
    ↓
crypto_sign_btc_hash() → ECDSA signature (64 bytes: r||s)
    ↓
memzero() → Clear sensitive buffers
    ↓
Response (USB WebUSB or HTTP JSON)
```

**Cryptographic Functions (via trezor-crypto):**
- **BIP-39:** Mnemonic generation from 128-bit entropy
- **BIP-32:** HD key derivation along path `m/44'/0'/0'/0/0`
- **ECDSA:** secp256k1 signing with RFC6979 deterministic k
- **SHA-256:** Transaction digest hashing
- **Entropy:** STM32 TRNG + software LCG pool

**Conditional Compilation:**
- `USE_CRYPTO_SIGN=1` → Full ECDSA, ~100-150 KB binary
- `USE_CRYPTO_SIGN=0` → SHA-256 only, ~5 KB binary
- `USE_TEST_SEED=1` → Reproducible test seed (no real security)

### 5. Networking (`task_net.c` / `app_ethernet_cw.c`)

**LwIP Integration:**
- IPv4 TCP/IP stack with DHCP client
- Fallback static IP: `192.168.0.10`
- SNTP time synchronization (updates via `time_service_set_epoch()`)
- HTTP server for `/tx` POST endpoint
- Optional WebUSB via `usb_device.c`

**Ethernet PHY:** Integrated into STM32H743, DMA buffers in D2 SRAM (cache must be disabled)

**State Machine:**
1. Link detection (PHY interrupt)
2. DHCP request
3. IP acquired
4. SNTP time request
5. Ready for transactions

### 6. I/O & User Interface (`task_io.c`)

**GPIO Mapping:**

| Function | Pin | Mode |
|---|---|---|
| LED Green | PB0 | Output |
| LED Yellow | PE1 | Output |
| LED Red | PE2 | Output |
| User Button | PC13 | Input (EXTI) |

**Debouncing:** Hardware EXTI with software debounce filter (20 ms)

### 7. USB & WebUSB (`usb_device.c` / `usb_webusb.c`)

**WebUSB Configuration:**
- VID/PID: `0x1209 / 0xC0DE`
- USB Speed: Full-speed (USB 1.1, 12 Mbps)
- BOS Descriptor with WebUSB Platform Capability UUID
- Serial number dynamically generated from STM32 DEVICE_ID registers

**Endpoints:**
- EP0: Control (default)
- EP1: Data-out (host → device)
- EP2: Data-in (device → host)

### 8. HAL Microcontroller Abstraction Layer

**MSP (Microcontroller Support Package) Files:**
- `stm32h7xx_hal_msp.c` — GPIO/clock/NVIC configuration for peripherals
- `stm32h7xx_it.c` — Interrupt handlers (SysTick, Ethernet IRQ)
- `stm32h7xx_it_usb.c` — OTG_HS IRQ handler for WebUSB
- `stm32h7xx_it_systick.c` — Alternative SysTick for minimal LwIP builds

## Security Architecture

### Memory Protection Unit (MPU)

- **Region 0:** Flash (code, read-only)
- **Region 1:** D1 SRAM (general data)
- **Region 2:** D2 SRAM (LwIP heap, cache disabled for DMA safety)
- **Region 3:** Peripheral I/O (device memory)

**Cache Settings:**
- D-Cache: Disabled for D2 (Ethernet DMA region)
- I-Cache: Enabled for flash (instruction fetch)

### Random Number Generation

**Entropy Mixing:**
1. STM32H743 hardware TRNG (`HAL_RNG_GenerateRandomNumber()`)
2. Software LCG entropy pool (Numerical Recipes parameters)
3. XOR combination for whitening

### Key Material Handling

- BIP-39 seed computed on-demand, never persisted
- Private keys allocated on stack with scope-limited lifetime
- `memzero()` clears all sensitive buffers before function return
- No global key storage in firmware flash

### Secure Features

- Deterministic ECDSA (RFC6979) prevents k-reuse attacks
- Base58Check validation for Bitcoin address integrity
- Input sanitization (regex, whitelist) for transaction requests

## Compilation & Build System

### Makefile Targets

```bash
make              # Build firmware
make clean        # Clean build artifacts
make docs-doxygen # Generate API docs (HTML/XML)
make flash        # Program STM32 via JTAG/SWD
make monitor      # UART debug console
```

### Build Configuration

```bash
# Example: Full featured build
make USE_LWIP=1 USE_WEBUSB=1 USE_CRYPTO_SIGN=1

# Minimal build (SHA-256 only)
make USE_CRYPTO_SIGN=0

# Test mode
make BOOT_TEST=1 USE_TEST_SEED=1
```

## Project Statistics

| Metric | Value |
|---|---|
| **Total Files** | ~50 C/C++ files |
| **Lines of Code** | ~15,000+ |
| **Core Modules** | 14 (main, display, IO, network, sign, security, USB, crypto, etc.) |
| **Documentation** | 43 markdown files (EN/RU/PL) |
| **Binary Size** | ~500 KB (full build with crypto) |
| **SRAM Usage** | ~100 KB (FreeRTOS stacks, queues) |
| **Project Size** | ~50 MB (incl. trezor-crypto) |

## Dependencies

### Hardware

- **MCU:** STM32H743ZI2 (ARM Cortex-M7, 480 MHz, 2 MB Flash, 512 KB SRAM)
- **Board:** Nucleo-H743ZI2
- **Display:** SSD1306 OLED 128×32 (I2C1)
- **Network:** Integrated Ethernet PHY + RJ45

### Software

- **STM32 HAL:** Hardware abstraction library
- **FreeRTOS:** CMSIS-RTOS2 API
- **LwIP:** Lightweight TCP/IP stack
- **trezor-crypto:** Bitcoin cryptographic library (MIT License)
- **SSD1306 Driver:** stm32-ssd1306 project

### External Projects

- `/data/projects/STM32CubeH7` — HAL drivers
- `/data/projects/stm32-ssd1306` — OLED driver library
- `/data/projects/stm32_secure_boot` — Linker scripts, FreeRTOS config
- `/data/projects/trezor-firmware` — Upstream trezor-crypto reference

## Documentation Structure

- **Architecture:** This file (high-level system design)
- **API Reference:** Doxygen-generated from source comments
- **Module Docs:** `docs_src/*.md` (detailed logic flow per file)
- **trezor-crypto Integration:** `docs_src/trezor-crypto-integration.md`
- **Hardware Config:** `docs_src/hw_init.md`, `docs_src/lwipopts.md`

## Next Steps & Future Enhancements

1. **Secure Element Integration:** Offload key storage to SE (e.g., ATECC608)
2. **Additional Coin Support:** Extend BIP-44 paths for Ethereum, Litecoin
3. **Bootloader:** Implement secure firmware update mechanism
4. **Formal Verification:** Security audit + cryptographic correctness proofs
5. **Production Hardening:** FIPS 140-2 compliance, side-channel resistance
