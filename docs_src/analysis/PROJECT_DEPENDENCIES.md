# Project Dependencies & Relationships

**Understanding how stm32_secure_boot and CryptoWallet relate**

---

## 🔗 Dependency Graph

```
External Dependencies (Third-party):
├─ STM32CubeH7/                    (ST Microelectronics HAL)
│  └─ Used by: [both projects]
│
├─ STM32CubeExpansion_Crypto/      (CMOX cryptographic library)
│  └─ Used by: stm32_secure_boot (bootloader)
│
├─ FreeRTOS kernel                 (RTOS)
│  ├─ Version in stm32_secure_boot/FreeRTOS/
│  └─ Used by: [both projects via stm32_secure_boot]
│
├─ LwIP network stack              (TCP/IP stack)
│  ├─ Version in stm32_secure_boot/ (lwip_zero profile)
│  └─ Used by: CryptoWallet (mandatory)
│
└─ trezor-crypto/                  (Bitcoin library)
   ├─ Location: CryptoWallet/ThirdParty/
   └─ Used by: CryptoWallet ONLY

Internal Dependencies:
stm32_secure_boot/
├─ Independent
├─ Can build standalone
├─ Provides bootloader + app examples
└─ Used as reference by CryptoWallet

            ↓ (USED AS FOUNDATION)

CryptoWallet/
├─ Depends on FreeRTOS from stm32_secure_boot
├─ Depends on LwIP (lwip_zero)
├─ Depends on HAL drivers
└─ Adds: trezor-crypto, HD wallets, full signing stack
```

---

## 📦 Component Dependency Tree

### stm32_secure_boot (Independent)

```
┌─ stm32_secure_boot (STANDALONE)
│
├─ bootloader/ ──┬─ Bootloader Core
│                ├─ SHA-256 (built-in)
│                ├─ ECDSA (CMOX or stub)
│                └─ Key storage
│
└─ app/ ─────────┬─ step2_hid/ (MAIN)
                 │  ├─ FreeRTOS
                 │  ├─ UART + USB HID
                 │  ├─ Button + OLED
                 │  └─ Signer transport
                 │
                 ├─ lwip_zero/
                 │  ├─ FreeRTOS
                 │  ├─ LwIP
                 │  └─ HTTP server
                 │
                 └─ step1, step2, etc.
```

### CryptoWallet (Composite)

```
┌─ CryptoWallet (DERIVATIVE)
│
├─ Dependencies:
│  ├─ FreeRTOS (from stm32_secure_boot)
│  ├─ LwIP (from stm32_secure_boot/lwip_zero)
│  ├─ STM32CubeH7 (HAL)
│  └─ trezor-crypto (ThirdParty/)
│
├─ Core Features:
│  ├─ Task-based architecture (FreeRTOS)
│  ├─ Multi-protocol (LwIP + WebUSB + UART)
│  ├─ HD Wallet (BIP-39/32)
│  ├─ TX Validation
│  ├─ Signing FSM
│  └─ RNG Testing ✨
│
└─ Does NOT include bootloader
   (Can inherit from stm32_secure_boot if desired)
```

---

## 📊 Code Sharing & Reuse Matrix

| Component | stm32_secure_boot | CryptoWallet | Notes |
|---|---|---|---|
| **FreeRTOS** | ✅ Included | ✅ Uses SB version | Shared kernel |
| **LwIP** | ✅ lwip_zero | ✅ Uses SB version | Network stack |
| **STM32 HAL** | ✅ External | ✅ External | Both use same HAL |
| **UART/USB/I2C drivers** | ✅ Included | ✅ Uses/extends SB | Partially shared |
| **Bootloader** | ✅ Full | ❌ Optional | SB advantage |
| **ECDSA Crypto** | ✅ CMOX | ✅ trezor-crypto | Different libs |
| **HD Wallet** | ❌ No | ✅ Yes | CW specific |
| **HTTP Server** | ✅ lwip_zero | ✅ Enhanced | CW improved |
| **WebUSB** | ❌ No | ✅ Yes | CW specific |
| **RNG Testing** | ❌ No | ✅ Yes | CW specific ✨ |

---

## 🔄 Build System Integration

### How to Build

#### stm32_secure_boot Standalone

```bash
cd /data/projects/stm32_secure_boot

# Bootloader
make bootloader
arm-none-eabi-objcopy -O binary build/bootloader.elf build/bootloader.bin

# Application (step2_hid)
make step2_hid
arm-none-eabi-objcopy -O binary build/app/app.elf build/app/app.bin

# Flash
st-flash write build/bootloader.bin 0x08000000
st-flash write build/app/app.bin 0x08010000
```

#### CryptoWallet With Optional Bootloader

```bash
cd /data/projects/CryptoWallet

# Option 1: Build with st32_secure_boot bootloader (recommended)
cd /data/projects/stm32_secure_boot && make bootloader
cp build/bootloader.bin ../CryptoWallet/build/

# Option 2: Build CryptoWallet only (app only)
cd /data/projects/CryptoWallet
make
arm-none-eabi-objcopy -O binary build/cryptowallet.elf build/cryptowallet.bin

# Flash (CryptoWallet at app address)
st-flash write build/cryptowallet.bin 0x08010000

# Or flash with bootloader
st-flash write build/bootloader.bin 0x08000000
```

---

## 🏗️ Memory Layout with Both Projects

### If Using stm32_secure_boot Bootloader + CryptoWallet App

```
Flash Memory Layout:
0x08000000 ┌──────────────────────────────────┐
           │  BOOTLOADER (from stm32_sb)      │  64 KB
           │  ├─ Bootloader code              │
           │  ├─ SHA-256 verification         │
           │  ├─ ECDSA verification (CMOX)    │
           │  ├─ Public keys                  │
           │  └─ LED/UART error reporting    │
0x08010000 ├──────────────────────────────────┤
           │  APPLICATION (CryptoWallet)      │  1.5 MB
           │  ├─ FreeRTOS kernel              │
           │  ├─ LwIP stack                   │
           │  ├─ trezor-crypto (Bitcoin)      │
           │  ├─ HTTP server                  │
           │  ├─ WebUSB interface             │
           │  ├─ Task managers                │
           │  │  ├─ task_sign.c               │
           │  │  ├─ task_net.c                │
           │  │  ├─ task_display.c            │
           │  │  ├─ task_user.c               │
           │  │  └─ task_io.c                 │
           │  ├─ RNG testing (NEW)            │
           │  └─ OLED driver (SSD1306)        │
0x08180000 ├──────────────────────────────────┤
           │  [Free space - unused]           │  512 KB
           │                                  │
0x08200000 └──────────────────────────────────┘

RAM Layout:
0x20000000 ┌──────────────────────────────────┐
           │  FreeRTOS Kernel                 │
           │  ├─ Task Control Blocks (TCB)    │  100 KB
           │  ├─ Ready lists                  │
           │  └─ Queue/event structures       │
0x20019000 ├──────────────────────────────────┤
           │  IPC Objects (Queues, Mutexes)   │
           │  ├─ tx_request_queue             │  50 KB
           │  ├─ sign_response_queue          │
           │  ├─ display_queue                │
           │  ├─ uart_log_queue               │
           │  └─ Mutexes/semaphores          │
0x20026000 ├──────────────────────────────────┤
           │  Task Stacks (5 tasks)           │  200 KB
           │  ├─ Sign task stack              │  (32 KB each)
           │  ├─ Network task stack           │
           │  ├─ Display task stack           │
           │  ├─ User task stack              │
           │  └─ IO task stack                │
0x20046000 ├──────────────────────────────────┤
           │  Heap (dynamic allocation)       │  250 KB
           │  ├─ LwIP buffers                 │
           │  ├─ malloc/free (crypto lib)     │
           │  └─ USB buffers                  │
0x200A2000 ├──────────────────────────────────┤
           │  LwIP RX descriptors + data      │  96 KB
           │  ├─ Ethernet frame buffer        │
           │  ├─ UDP/TCP buffers              │
           │  └─ mbuf chains                  │
0x200C0000 ├──────────────────────────────────┤
           │  [Remaining space]               │  192 KB
           │                                  │
0x20100000 └──────────────────────────────────┘
```

---

## 🔐 Security Chain of Trust

### With stm32_secure_boot Bootloader

```
┌─────────────────────────────────────────┐
│  STM32H743 System Flash @ 0x08000000    │
│                                          │
│  ┌──────────────────────────────────┐  │
│  │  ARM Cortex-M7 ROM Bootloader    │  │
│  │  (1 KB, immutable from factory)  │  │
│  │                                   │  │
│  │  Responsibility:                 │  │
│  │  └─ Load & execute user         │  │
│  │     bootloader @ 0x08000000     │  │
│  └──────────────────────────────────┘  │
│            ↓ Jumps to ↓                 │
│  ┌──────────────────────────────────┐  │
│  │  User Bootloader (64 KB)         │  │
│  │  (from stm32_secure_boot)        │  │
│  │                                   │  │
│  │  Responsibility:                 │  │
│  │  ├─ Compute SHA-256 of app      │  │
│  │  ├─ Verify ECDSA signature      │  │
│  │  ├─ Jump to app if OK            │  │
│  │  └─ Halt + LED error if bad      │  │
│  └──────────────────────────────────┘  │
│            ↓ Verified Jump ↓            │
│  ┌──────────────────────────────────┐  │
│  │  Application (1.5 MB)            │  │
│  │  (from CryptoWallet)             │  │
│  │                                   │  │
│  │  Responsibility:                 │  │
│  │  ├─ Initialize hardware          │  │
│  │  ├─ Create FreeRTOS tasks        │  │
│  │  ├─ Wait for user input          │  │
│  │  ├─ Sign transactions            │  │
│  │  └─ Return signatures            │  │
│  └──────────────────────────────────┘  │
└─────────────────────────────────────────┘

Trust Chain:
ROM Bootloader
       ↓ trusts
User Bootloader (signed with ECDSA)
       ↓ trusts
Application (signed with ECDSA)
       ↓ executes
CryptoWallet Firmware
```

---

## 🎯 Integration Scenarios

### Scenario 1: Standalone stm32_secure_boot

```
Use Case: Testing bootloader + HID signer
├─ Build: make step2_hid
├─ Deploy: Single binary with bootloader + app
├─ Result: Bootloader-verified application
└─ Advantage: Verified boot chain, but no wallet features
```

### Scenario 2: CryptoWallet Only (No Bootloader)

```
Use Case: Wallet without bootloader verification
├─ Build: make
├─ Deploy: CryptoWallet binary @ 0x08010000
├─ Flash: Direct to 0x08010000 (skip 0x08000000)
└─ Advantage: More space for application, simpler
   Disadvantage: No boot verification
```

### Scenario 3: stm32_secure_boot Bootloader + CryptoWallet (RECOMMENDED)

```
Use Case: Production wallet with boot verification
├─ Build:
│  1. cd stm32_secure_boot && make bootloader
│  2. cd CryptoWallet && make
├─ Deploy:
│  1. Flash bootloader.bin @ 0x08000000
│  2. Sign CryptoWallet app (optional)
│  3. Flash cryptowallet.bin @ 0x08010000
├─ Result: Verified boot chain + full wallet features
└─ Advantage: Maximum security + functionality
   Process: More complex, requires signing
```

### Scenario 4: Development Workflow

```
Phase 1: Bootloader Development
├─ cd stm32_secure_boot
├─ make bootloader
├─ make flash-bootloader
└─ Test with step2_hid

Phase 2: Application Development
├─ cd CryptoWallet
├─ make
├─ make flash
└─ Test signing/network

Phase 3: Integration Testing
├─ Both bootloader + app
├─ Full chain verification
├─ Security hardening
└─ Production release
```

---

## 📈 Code Metrics Comparison

### stm32_secure_boot

```
├─ Total Files:           ~150 files
├─ Total Lines of Code:   ~50,000 LOC
│  ├─ Bootloader:         ~3,000 LOC
│  ├─ App (step2_hid):    ~8,000 LOC
│  ├─ FreeRTOS:          ~12,000 LOC
│  ├─ LwIP:              ~20,000 LOC
│  └─ Common/Examples:    ~7,000 LOC
│
├─ Build Profiles:        12+
├─ Documentation:         20+ files
└─ Status:                Research/Educational
```

### CryptoWallet

```
├─ Total Files:           ~200 files (including docs)
├─ Total Lines of Code:   ~60,000+ LOC
│  ├─ Core:               ~8,000 LOC
│  ├─ Tasks:              ~5,000 LOC
│  ├─ Crypto/Wallet:      ~3,000 LOC
│  ├─ Network/USB:        ~4,000 LOC
│  ├─ FreeRTOS:          ~12,000 LOC
│  ├─ LwIP:              ~20,000 LOC
│  └─ Documentation:      ~8,000 LOC
│
├─ Build Variants:        3 (main, minimal, test)
├─ Python Scripts:        11+ test/utility scripts
├─ Documentation:         128+ markdown files (EN/RU/PL)
├─ Test Infrastructure:   Complete (NEW) ✨
└─ Status:                Production-Ready
```

---

## 🚀 How to Integrate stm32_secure_boot into CryptoWallet

### Method 1: Use Existing FreeRTOS/LwIP (Current)

CryptoWallet already uses the FreeRTOS and LwIP from stm32_secure_boot:

```bash
# Implicit dependency
cd /data/projects/CryptoWallet

# FreeRTOS and LwIP are references to stm32_secure_boot versions
# (if not included directly in CryptoWallet)
```

### Method 2: Adopt stm32_secure_boot Bootloader

```bash
# Option A: Copy bootloader to CryptoWallet repo
cp -r /data/projects/stm32_secure_boot/bootloader \
      /data/projects/CryptoWallet/

# Option B: Reference as submodule
cd /data/projects/CryptoWallet
git submodule add /data/projects/stm32_secure_boot bootloader

# Option C: Modify CryptoWallet Makefile
# Add bootloader build target
cat >> Makefile << 'EOF'
bootloader:
    $(MAKE) -C ../stm32_secure_boot bootloader
    cp ../stm32_secure_boot/build/bootloader.bin build/
EOF
```

### Method 3: Use RNG Testing from CryptoWallet in stm32_secure_boot

```bash
# Copy RNG testing infrastructure
cp /data/projects/CryptoWallet/scripts/test_rng_*.py \
   /data/projects/stm32_secure_boot/scripts/

# Copy documentation
cp -r /data/projects/CryptoWallet/docs_src/crypto/rng* \
      /data/projects/stm32_secure_boot/docs/

# Now stm32_secure_boot can also test RNG
cd /data/projects/stm32_secure_boot
make USE_RNG_DUMP=1
python3 scripts/test_rng_signing_comprehensive.py
```

---

## 🔧 Build Dependency Resolution

### Makefile Dependency Checking

```makefile
# How CryptoWallet ensures FreeRTOS/LwIP availability

FREERTOS_PATH ?= ../stm32_secure_boot/FreeRTOS
LWIP_PATH ?= ../stm32_secure_boot

check-deps:
    @if [ ! -d "$(FREERTOS_PATH)" ]; then \
        echo "ERROR: FreeRTOS not found at $(FREERTOS_PATH)"; \
        exit 1; \
    fi
    @if [ ! -d "$(LWIP_PATH)" ]; then \
        echo "ERROR: LwIP not found at $(LWIP_PATH)"; \
        exit 1; \
    fi

all: check-deps compile link
```

---

## 📋 Dependency Verification Checklist

```bash
# Verify all dependencies are available

✅ STM32CubeH7
   └─ Location: /data/projects/STM32CubeH7/
   └─ Check: [ -d STM32CubeH7/Drivers ] && echo "OK"

✅ FreeRTOS
   └─ Location: /data/projects/stm32_secure_boot/FreeRTOS/
   └─ Check: [ -f FreeRTOS/Source/tasks.c ] && echo "OK"

✅ LwIP
   └─ Location: /data/projects/stm32_secure_boot/
   └─ Check: [ -d lwip ] && echo "OK"

✅ trezor-crypto
   └─ Location: /data/projects/CryptoWallet/ThirdParty/trezor-crypto/
   └─ Check: [ -f crypto.h ] && echo "OK"

✅ Toolchain
   └─ Check: arm-none-eabi-gcc --version
   └─ Should: >= 11.0

✅ Build Tools
   └─ Check: which make && which st-flash
   └─ Required: GNU make, ST-Link utils
```

---

**Document:** Project Dependencies & Relationships  
**Updated:** 2026-03-20  
**Status:** Complete
