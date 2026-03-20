# Architecture & Technology Stack Comparison

**Visual Guide: stm32_secure_boot vs CryptoWallet**

---

## 🏛️ System Architecture Comparison

### stm32_secure_boot Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     HARDWARE LAYER                              │
│  STM32H743 (Cortex-M7 @ 480 MHz, 2MB Flash, 1MB SRAM)         │
│  Peripherals: UART, USB, I2C, GPIO, RNG (optional), Ethernet   │
└─────────────────────────────────────────────────────────────────┘

                    ↓ Boot Sequence ↓

┌─────────────────────────────────────────────────────────────────┐
│          BOOTLOADER (64 KB @ 0x08000000)                        │
│                                                                  │
│  ┌─ Startup Code (ARM Cortex-M7 assembly)                     │
│  ├─ Clock Configuration (PLL, AHB dividers)                   │
│  ├─ HAL Initialization                                         │
│  ├─ SHA-256 Compute (Image Hash)                              │
│  │   └─ hash = SHA256(app_image)                              │
│  ├─ ECDSA Signature Verification                              │
│  │   ├─ With CMOX library (real crypto)                       │
│  │   └─ Or STUB mode (for testing)                            │
│  ├─ Verification Decision                                      │
│  │   ├─ IF signature valid → LED green, jump to app          │
│  │   └─ IF signature invalid → LED red, halt                 │
│  └─ Jump to Application                                        │
│       └─ jmp 0x08010000 (Application Address)                │
└─────────────────────────────────────────────────────────────────┘

                    ↓ Verified ↓

┌─────────────────────────────────────────────────────────────────┐
│          APPLICATION (256+ KB @ 0x08010000)                     │
│                                                                  │
│  ╔══════════════════════════════════════════════════════════╗  │
│  ║           FreeRTOS KERNEL                               ║  │
│  ║  Scheduler, Tasks, Queues, Mutexes, Semaphores         ║  │
│  ╚══════════════════════════════════════════════════════════╝  │
│                                                                  │
│  Task 1: Signing FSM          Task 2: UI/Display             │
│  ├─ UART receive              ├─ I2C (SSD1306)             │
│  ├─ Command parsing           ├─ 128x32 pixels             │
│  ├─ Key derivation            └─ 4-line text rendering    │
│  ├─ SHA-256 compute                                        │
│  ├─ ECDSA sign                Task 3: Button Input         │
│  ├─ Response queue            ├─ Debouncing (30ms)        │
│  └─ Next state                └─ Confirm/Reject           │
│                                                             │
│  Optional: LwIP (lwip_zero)   Optional: USB HID            │
│  ├─ Ethernet                  ├─ 64-byte reports          │
│  ├─ IP Stack                  ├─ Report IN/OUT            │
│  └─ HTTP Server               └─ Application protocol     │
└─────────────────────────────────────────────────────────────────┘
```

**Key Data Flow:**
```
Input (UART/HID) → Signer Transport → Command Handler
    ↓
Key Management (In-memory)
    ↓
SHA-256 Hash → ECDSA Sign
    ↓
Response (UART/HID/Display)
```

---

### CryptoWallet Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     HARDWARE LAYER                              │
│  STM32H743 (Cortex-M7 @ 480 MHz, 2MB Flash, 1MB SRAM)         │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ UART (USART3)    │  │ USB (Device)     │  │ Ethernet     │ │
│  │ 115200 baud      │  │ FS @ 12 Mbps     │  │ RMII Mode    │ │
│  │ Debug/Control    │  │ WebUSB Protocol  │  │ DHCP Client  │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ I2C (I2C1)       │  │ SPI (if needed)  │  │ RNG (AES)    │ │
│  │ OLED SSD1306     │  │ Flash storage    │  │ 32-bit words │ │
│  │ 128×32 display   │  │ (optional)       │  │ Statistical  │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ GPIO: Button (PC13), LEDs (indicator)                      │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Firmware ↓

┌─────────────────────────────────────────────────────────────────┐
│               BOOTLOADER (External or Internal)                 │
│  Optional: SHA-256 + ECDSA verification (from stm32_secure_boot)
└─────────────────────────────────────────────────────────────────┘

                        ↓ Boot ↓

┌─────────────────────────────────────────────────────────────────┐
│                    HARDWARE INIT LAYER                          │
│  hw_init.c                                                      │
│  ├─ Clock configuration (PLL, HSI48)                           │
│  ├─ GPIO setup (UART TX/RX, I2C SDA/SCL, USB, Ethernet)      │
│  ├─ UART initialization (USART3 @ 115200)                     │
│  ├─ I2C initialization (100 kHz for OLED)                     │
│  ├─ USB device initialization (FS @ 12 Mbps)                  │
│  ├─ Ethernet PHY initialization (RMII)                        │
│  ├─ RNG initialization (if USE_RNG_DUMP=1) ⭐ NEW             │
│  └─ NVIC setup (interrupt priorities)                         │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Create OS ↓

┌─────────────────────────────────────────────────────────────────┐
│             FREERTOS KERNEL + IPC OBJECTS                       │
│                                                                  │
│  ╔══════════════════════════════════════════════════════════╗  │
│  ║         FreeRTOS Scheduler & Kernel                     ║  │
│  ║  - Task creation                                        ║  │
│  ║  - Context switching (every 1 ms SysTick)             ║  │
│  ║  - Queue management                                    ║  │
│  ║  - Mutex/semaphore handling                           ║  │
│  ╚══════════════════════════════════════════════════════════╝  │
│                                                                  │
│  Queues:                                                        │
│  ├─ tx_request_queue    (HTTP/USB → Sign task)                │
│  ├─ sign_response_queue  (Sign task → Net task)               │
│  ├─ display_queue        (All tasks → Display update)         │
│  └─ uart_log_queue       (All tasks → UART output)            │
│                                                                  │
│  Mutex: crypto_lock (protects trezor-crypto library)          │
│  Events: user_confirm_event (button press)                    │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Create Tasks ↓

┌─────────────────────────────────────────────────────────────────┐
│                    PROTOCOL/DRIVER TASKS                        │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Network Task (task_net.c) - Priority: 20                ║ │
│  ║  HTTP Server (port 80) over LwIP                        ║ │
│  ║  ├─ Receive: POST /tx (JSON or form data)             ║ │
│  ║  ├─ Parse:  {amount, address, currency}               ║ │
│  ║  ├─ Enqueue: tx_request → tx_request_queue             ║ │
│  ║  └─ Send: HTTP 200 OK or error                        ║ │
│  ║                                                        ║ │
│  ║  WebUSB Interface (usb_webusb.c)                      ║ │
│  ║  ├─ Bulk IN: Signature response                       ║ │
│  ║  └─ Bulk OUT: TX request                              ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Signing Task (task_sign.c) - Priority: 21               ║ │
│  ║  IPC-driven signing FSM ⭐ Key component               ║ │
│  ║  ├─ Wait: on tx_request_queue                          ║ │
│  ║  ├─ Validate: address (Base58/bech32)                 ║ │
│  ║  ├─ Derive: key from seed (BIP-32)                    ║ │
│  ║  ├─ Wait: user confirmation (button)                  ║ │
│  ║  ├─ Sign: SHA-256 + ECDSA (trezor-crypto)             ║ │
│  ║  ├─ Zero: sensitive buffers (memzero)                 ║ │
│  ║  ├─ Send: signature → sign_response_queue             ║ │
│  ║  └─ Loop                                               ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Display Task (task_display.c) - Priority: 15            ║ │
│  ║  SSD1306 OLED Management (128×32 pixels)               ║ │
│  ║  ├─ Render: 4-line text + scrolling log                ║ │
│  ║  ├─ State: "Waiting...", "Signing...", "OK", "ERR"   ║ │
│  ║  ├─ Update: @ 10 Hz (every 100 ms)                    ║ │
│  ║  └─ Read: display_queue for updates                   ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ User Task (task_user.c) - Priority: 22 (highest)       ║ │
│  ║  Button & Keyboard Input Handler                       ║ │
│  ║  ├─ Poll: PC13 button (20 ms intervals)               ║ │
│  ║  ├─ Debounce: 30 ms stable detection                  ║ │
│  ║  ├─ Signal: user_confirm_event → Signing task        ║ │
│  ║  └─ LED: status feedback                              ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ IO Task (task_io.c) - Priority: 18                      ║ │
│  ║  LED Indicator Management                              ║ │
│  ║  ├─ Green: Normal operation                            ║ │
│  ║  ├─ Red: Error state                                   ║ │
│  ║  ├─ Blue: Network activity                            ║ │
│  ║  └─ Blink patterns: Status indication                 ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  Optional RNG Task (NEW) ⭐                                    │
│  ├─ Read: RNG peripheral @ 6 MHz                            │
│  ├─ Output: UART @ 115200                                    │
│  └─ Use: Dieharder statistical testing                       │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Protocol Stacks ↓

┌─────────────────────────────────────────────────────────────────┐
│         COMMUNICATION PROTOCOL LAYERS                           │
│                                                                  │
│  LwIP Stack (Ethernet)                                          │
│  ├─ MAC: RMII interface (ETH1/2)                              │
│  ├─ IP: DHCP or static                                        │
│  ├─ TCP/UDP: Full stack                                       │
│  └─ HTTP: Simple server (port 80)                            │
│                                                                  │
│  USB Stack                                                      │
│  ├─ FS Device (12 Mbps)                                       │
│  ├─ CDC ACM: Virtual serial (optional)                       │
│  ├─ WebUSB: Custom bulk endpoints ⭐                         │
│  └─ Descriptors: BOS Platform Capability                     │
│                                                                  │
│  UART Protocol                                                  │
│  └─ Debug/fallback communications                             │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Cryptography ↓

┌─────────────────────────────────────────────────────────────────┐
│          CRYPTOGRAPHY LAYER (trezor-crypto)                    │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Key Management                                          ║ │
│  ║  Seed Storage:                                          ║ │
│  ║  ├─ Option 1: Test seed (hardcoded for development)   ║ │
│  ║  ├─ Option 2: BIP-39 mnemonic (12 or 24 words)        ║ │
│  ║  └─ Stored in RAM (cleared on power loss)             ║ │
│  ║                                                        ║ │
│  ║  Key Derivation (BIP-32):                             ║ │
│  ║  ├─ Master key from seed                              ║ │
│  ║  ├─ Path: m/44'/0'/0'/0/0 (BIP-44 standard)          ║ │
│  ║  ├─ Derive: child public/private keys                 ║ │
│  ║  └─ Per-address keys (not reused)                     ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Hashing                                                 ║ │
│  ║  ├─ SHA-256: Transaction digest                        ║ │
│  ║  ├─ HMAC-SHA-512: BIP-32 key derivation              ║ │
│  ║  └─ RIPEMD-160: Address generation (Bitcoin)         ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Signing (ECDSA)                                         ║ │
│  ║  ├─ Curve: secp256k1 (Bitcoin standard)                ║ │
│  ║  ├─ Hash: SHA-256(transaction)                         ║ │
│  ║  ├─ Nonce: Generated per signature (k parameter)       ║ │
│  ║  ├─ Signature: (r, s) pair                            ║ │
│  ║  └─ Encoding: DER format                              ║ │
│  ║                                                        ║ │
│  ║  Security Properties:                                 ║ │
│  ║  ├─ Deterministic (RFC 6979) or random                ║ │
│  ║  ├─ Vulnerable if k is compromised                    ║ │
│  ║  └─ Therefore: RNG quality critical ⭐ NEW             ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Memory Safety                                           ║ │
│  ║  ├─ memzero(): Secure buffer clearing                  ║ │
│  ║  ├─ Prevents compiler optimization                    ║ │
│  ║  ├─ Applied to: seeds, keys, signatures              ║ │
│  ║  └─ Critical for reducing key leakage                 ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Validation ↓

┌─────────────────────────────────────────────────────────────────┐
│         TRANSACTION VALIDATION LAYER                           │
│                                                                  │
│  Input: Bitcoin TX request {amount, address, currency}         │
│                                                                  │
│  Validation Steps:                                              │
│  1. Address Format Check                                        │
│     ├─ Base58Check (P2PKH: 1...)                              │
│     ├─ Bech32 (SegWit: bc1...)                               │
│     └─ Return error if invalid                               │
│                                                                  │
│  2. Amount Validation                                          │
│     ├─ Range check (> 0, < 21M BTC)                         │
│     ├─ Fee estimation                                        │
│     └─ Return error if invalid                              │
│                                                                  │
│  3. Currency Check                                             │
│     ├─ Support: BTC (mainnet/testnet)                       │
│     └─ Return error if unsupported                          │
│                                                                  │
│  Output: Validated TX or error message                        │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Output ↓

┌─────────────────────────────────────────────────────────────────┐
│              USER INTERFACE LAYER                              │
│                                                                  │
│  SSD1306 OLED (128×32 pixels)                                  │
│  ┌────────────────────────────────────────┐                    │
│  │ CryptoWallet v1.0                  [OK]│                    │
│  │ Signing tx...                          │                    │
│  │ Amount: 1.5 BTC → 1A1z...            │                    │
│  │ Confirm? (Press button)                │                    │
│  └────────────────────────────────────────┘                    │
│                                                                  │
│  Button (PC13)                                                  │
│  └─ Confirm: Press to sign                                    │
│  └─ Reject: Press+hold for 2 sec                              │
│                                                                  │
│  LED Status                                                     │
│  ├─ Green (500 ms): Ready                                     │
│  ├─ Blue (50 ms): Network activity                            │
│  ├─ Red (100 ms): Error                                       │
│  └─ Off: Power save                                           │
│                                                                  │
│  UART Output (debug)                                           │
│  └─ All activity logged for troubleshooting                   │
└─────────────────────────────────────────────────────────────────┘
```

**Key Data Flow (Full TX Signing):**
```
HTTP POST /tx?amount=1.5&address=1A1z...
    ↓
task_net.c (parse JSON/form)
    ↓
tx_request_queue (IPC)
    ↓
task_sign.c (validate, derive key, wait for button)
    ↓
User presses button
    ↓
ECDSA sign (trezor-crypto)
    ↓
memzero() buffers
    ↓
sign_response_queue (IPC)
    ↓
task_net.c (return signature)
    ↓
HTTP 200 + {signature, tx_id}
```

---

## 📊 Technology Stack Matrix

| Layer | stm32_secure_boot | CryptoWallet |
|---|---|---|
| **CPU** | STM32H743 (Cortex-M7, 480 MHz) | STM32H743 (Cortex-M7, 480 MHz) |
| **OS** | FreeRTOS (8.2.3 or newer) | FreeRTOS (8.2.3+) |
| **Memory** | 2 MB Flash / 1 MB SRAM | 2 MB Flash / 1 MB SRAM |
| **Build** | GNU Make + arm-none-eabi-gcc | GNU Make + arm-none-eabi-gcc |
| **Bootloader** | ✅ SHA-256 + ECDSA (CMOX) | ❌ (optional) |
| **Network Stack** | LwIP (lwip_zero) | LwIP (mandatory) |
| **Crypto Library** | CMOX (ST Micro) | trezor-crypto (Satoshi Labs) |
| **HD Wallet** | ❌ | ✅ BIP-39/BIP-32 |
| **USB** | ✅ HID (64-byte reports) | ✅ WebUSB (bulk endpoints) |
| **Protocol** | Dual (UART + HID) | Triple (UART + WebUSB + HTTP) |
| **Display** | Optional SSD1306 | Mandatory SSD1306 |
| **RNG Testing** | ❌ | ✅ Dieharder suite (NEW) |
| **Documentation** | Polish/Russian | English/Russian/Polish |

---

## 🔗 Communication Protocols Comparison

### stm32_secure_boot - UART Transport

```
Host Command Format:
┌─────────────────────────────────┐
│ Frame Header: "CMD:"            │
│ Command: PING, PONG, STATUS,... │
│ Parameters: [optional]          │
│ Terminator: \r\n               │
└─────────────────────────────────┘

Example:
  Host → Device: "CMD:SIGN\r\n"
  Device → Host: "PONG:OK\r\n"
```

### CryptoWallet - HTTP Protocol

```
POST /tx HTTP/1.1
Host: 192.168.1.100
Content-Type: application/x-www-form-urlencoded

amount=1.5&address=1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa&currency=BTC

HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "signed",
  "signature": "304402201f2...",
  "tx_id": "abc123def456..."
}
```

### CryptoWallet - WebUSB Protocol

```
WebUSB Frame Format:
┌──────────────────────────────────┐
│ Header (4 bytes): frame type     │
│ Length (2 bytes): payload length │
│ Payload (0-58 bytes): data      │
│ CRC (2 bytes): error checking   │
└──────────────────────────────────┘

Typical flow:
1. Host: Ping frame
2. Device: Pong response
3. Host: TX request (JSON)
4. Device: Signature response
```

---

## 💾 Memory Layout Comparison

### stm32_secure_boot Layout

```
0x08000000 ┌─────────────────────┐
           │  BOOTLOADER         │  64 KB
           │  SHA-256 + ECDSA    │
           │  Keys storage       │
0x08010000 ├─────────────────────┤
           │  APPLICATION        │  
           │  FreeRTOS           │  1.5 MB
           │  LwIP (optional)    │
           │  Crypto (CMOX)      │
0x08180000 ├─────────────────────┤
           │  [unused]           │  512 KB
           │                     │
0x08200000 └─────────────────────┘

RAM Layout:
0x20000000 ┌─────────────────────┐
           │  FreeRTOS TCB       │  ~100 KB
           │  & Kernel objects   │
0x20018000 ├─────────────────────┤
           │  Task Stacks        │  ~400 KB
           │  (5-6 tasks)        │
0x20080000 ├─────────────────────┤
           │  Heap               │  ~500 KB
           │  (malloc/free)      │
0x200F8000 └─────────────────────┘
```

### CryptoWallet Layout

```
0x08000000 ┌─────────────────────┐
           │  BOOTLOADER         │  64 KB (optional)
           │  (from stm32_sb)    │
0x08010000 ├─────────────────────┤
           │  APPLICATION        │  
           │  FreeRTOS           │  1.5 MB
           │  LwIP (mandatory)   │
           │  trezor-crypto      │
           │  HTTP server        │
           │  WebUSB             │
           │  RNG testing code   │  ✨ NEW
0x08180000 ├─────────────────────┤
           │  [unused]           │  512 KB
           │                     │
0x08200000 └─────────────────────┘

RAM Layout (with crypto):
0x20000000 ┌─────────────────────┐
           │  FreeRTOS TCB       │  ~150 KB
           │  & IPC objects      │
0x20025800 ├─────────────────────┤
           │  Queues & Mutexes   │  ~50 KB
           │  (tx, sign, display)│
0x20032000 ├─────────────────────┤
           │  Task Stacks        │  ~300 KB
           │  (5 tasks)          │
0x20080000 ├─────────────────────┤
           │  Heap               │  ~400 KB
           │  (malloc for crypto)│
0x200F8000 ├─────────────────────┤
           │  LwIP + RX buffers  │  ~64 KB
0x20100000 └─────────────────────┘
```

---

**Document:** Architecture & Technology Stack Comparison  
**Last Updated:** 2026-03-20  
**Status:** Complete
