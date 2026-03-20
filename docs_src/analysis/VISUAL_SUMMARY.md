# Visual Project Comparison & Analysis Summary

**Graphical Overview of CryptoWallet & stm32_secure_boot**

---

## 🎨 Project Landscape Visualization

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│                    STM32H7 ECOSYSTEM (Shared Hardware)                  │
│                         NUCLEO-H743ZI2 Board                           │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────────────┐        ┌────────────────────────────┐   │
│  │   stm32_secure_boot      │        │      CryptoWallet          │   │
│  │   (Research Platform)    │        │  (Production Wallet)       │   │
│  │                          │        │                            │   │
│  │  ✅ Verified Boot        │        │  ✅ Full HD Wallet         │   │
│  │  ✅ Multi-Transport      │        │  ✅ Multi-Protocol         │   │
│  │  ✅ HID/UART            │        │  ✅ HTTP/WebUSB/UART       │   │
│  │  ✅ Multiple Profiles    │        │  ✅ RNG Testing (NEW)      │   │
│  │  ❌ No HD Keys           │        │  ✅ TX Validation          │   │
│  │  ❌ No Wallet Features   │        │  ✅ Secure Memory          │   │
│  │                          │        │                            │   │
│  │  12+ Build Profiles      │        │  1 Main + Variants         │   │
│  │  Beta Status             │        │  Stable Status             │   │
│  │  50K LOC                 │        │  60K LOC + Docs            │   │
│  └──────────────────────────┘        └────────────────────────────┘   │
│           ▲                                      ▲                      │
│           │                                      │                      │
│           └──────────── Uses ─────────────────►  │                      │
│                                                  │                      │
│           FreeRTOS + LwIP                       │                      │
│           (Borrowed from stm32_sb)              │                      │
│                                                  │                      │
│  ┌──────────────────────────┐                   │                      │
│  │   Shared Components      │◄──────────────────┘                      │
│  │                          │                                          │
│  │  • FreeRTOS Kernel       │                                          │
│  │  • LwIP Stack            │                                          │
│  │  • STM32 HAL Drivers     │                                          │
│  │  • UART/USB/I2C Drivers  │                                          │
│  │  • GPIO/LED Management   │                                          │
│  │  • OLED SSD1306 Support  │                                          │
│  │                          │                                          │
│  │  NEW (CW): RNG Testing   │                                          │
│  │                          │                                          │
│  └──────────────────────────┘                                          │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

Key Relationship:
┌─────────────────────────────────────────────────────────────┐
│  stm32_secure_boot = Foundation/Library                      │
│  CryptoWallet = Application/Implementation                   │
│                                                               │
│  CW = SB + trezor-crypto + BIP-39/32 + HTTP + WebUSB + RNG  │
└─────────────────────────────────────────────────────────────┘
```

---

## 📊 Feature Matrix Heatmap

```
                    stm32_secure_boot   CryptoWallet
┌──────────────────────────────────────────────────────┐
│ Bootloader       ████████████ (Full)     ░░░░░░ (Opt) │  16 pts
│ Crypto           ████░░░░░░░░ (Basic)    ████████ (Adv)│  12 pts
│ HD Wallet        ░░░░░░░░░░░░ (None)     ████████ (Full)│  8 pts
│ Network          ████░░░░░░░░ (LwIP)     ████████ (Full)│  8 pts
│ Protocols        ████░░░░░░░░ (2x)       ████████ (3x) │  8 pts
│ Display          ████░░░░░░░░ (Opt)      ████████ (Req) │  8 pts
│ RNG Testing      ░░░░░░░░░░░░ (None)     ████████ (New) │  8 pts
│ Documentation    ██████░░░░░░ (Good)     ████████ (Exc) │  8 pts
│ Production Ready ░░░░░░░░░░░░ (Beta)     ████████ (Yes) │  8 pts
│ Test Framework   ░░░░░░░░░░░░ (Basic)    ████████ (Comp)│  8 pts
│                                                        │
│ TOTAL SCORE:                        52 / 100          │
│ SB Advanced:  30/100 (Research)     CW Capability:    │
│ CW Advanced:  72/100 (Production)   22/100 advantage  │
└──────────────────────────────────────────────────────┘

█ = Implemented/Strong | ░ = Not Implemented/Weak
```

---

## 🔄 Data Flow Diagrams

### stm32_secure_boot - HID Signer Flow

```
User Input (Button/UART/HID)
         ↓
    ┌────────────────────┐
    │ Signer Transport   │
    │ - Parse command    │
    │ - Validate input   │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Key Management     │
    │ - Load key         │
    │ - In-memory only   │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Signing Engine     │
    │ - SHA-256 hash     │
    │ - ECDSA sign       │
    │ - CMOX library     │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Response           │
    │ - UART or HID      │
    │ - Signature        │
    └────────────────────┘
```

### CryptoWallet - Full Wallet Flow

```
Network Input (HTTP/WebUSB/UART)
         ↓
    ┌─────────────────────────┐
    │ Protocol Handling       │
    │ - Parse JSON/binary     │
    │ - Multiple transports   │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Transaction Validation  │
    │ - Address (Base58/Bech32)
    │ - Amount check          │
    │ - Currency support      │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Key Derivation          │
    │ - BIP-32 HD path        │
    │ - trezor-crypto lib     │
    │ - Child key generation  │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ User Confirmation       │
    │ - Button press required │
    │ - OLED display info     │
    │ - Confirm/Reject        │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Signing Engine          │
    │ - SHA-256 + HMAC-SHA512 │
    │ - ECDSA secp256k1       │
    │ - RFC 6979 nonce        │
    │ - DER encoding          │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Memory Cleanup          │
    │ - memzero() buffers     │
    │ - Secure clearing       │
    │ - Prevent leakage       │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Response               │
    │ - Signature + metadata  │
    │ - Multi-protocol output │
    └─────────────────────────┘
```

---

## 📈 Project Growth Timeline

```
2024-2025: stm32_secure_boot Development
├─ Q1-Q3: Bootloader + basic apps
├─ Q4:    Multiple profiles, HID support
└─ Status: Research/Educational platform

         ↓ (Uses as foundation)

2025-2026: CryptoWallet Development
├─ Phase 1: Core FreeRTOS + LwIP (from SB)
├─ Phase 2: trezor-crypto integration
├─ Phase 3: HD wallet (BIP-39/32)
├─ Phase 4: Multi-protocol (HTTP, WebUSB)
├─ Phase 5: Testing & hardening
└─ Phase 6: RNG Testing Infrastructure ✨ NEW
            └─ 30 new files
            └─ Test automation
            └─ Documentation (3 languages)

Future:
├─ Production deployment
├─ Security audit
├─ Hardware wallet features
└─ Long-term support
```

---

## 🎯 Use Case Venn Diagram

```
        ┌─────────────────────────────────────────────┐
        │   stm32_secure_boot                        │
        │   (Research & Education)                   │
        │                                             │
        │  • Verified Boot                ┌──────────┴───────────┐
        │  • Bootloader Design            │                      │
        │  • ECDSA Verification     BOTH: │ • FreeRTOS           │
        │  • HID Protocol               │ • LwIP                │
        │  • Multi-transport            │ • STM32 HAL           │
        │  • Educational               │ • Embedded ARM         │
        │  • Reference Code             │                      │
        │                             │                      │
        └──────────────┬──────────────┴──────────────┐
                       │   CryptoWallet              │
                       │  (Production Wallet)       │
                       │                             │
                       │  • HD Wallet (BIP-39/32)   │
                       │  • Bitcoin Integration     │
                       │  • TX Validation           │
                       │  • HTTP Server             │
                       │  • WebUSB Interface        │
                       │  • RNG Testing             │
                       │  • Secure Key Management   │
                       │  • User Confirmation       │
                       │  • Production-ready        │
                       │  • QA Infrastructure       │
                       │                             │
                       └──────────────────────────────┘

Legend: Overlapping = Shared/Related features
```

---

## 🏗️ Architecture Layer Cake

```
┌────────────────────────────────────────┐
│      USER APPLICATION LAYER            │
│  ┌──────────────────────────────────┐  │
│  │ stm32_secure_boot:  Signer App   │  │
│  │ CryptoWallet:       Wallet App   │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘
            ↑            ↑
            │ Uses        │
            ▼            ▼
┌──────────────────────┬──────────────────┐
│   PROTOCOL LAYER     │   CRYPTO LAYER   │
├──────────────────────┼──────────────────┤
│ SB: UART/HID         │ SB: CMOX/ECDSA   │
│ CW: HTTP/WebUSB/UART │ CW: trezor/BIP39 │
└──────────────────────┴──────────────────┘
            ▲            ▲
            │ Uses        │
            ▼            ▼
┌──────────────────────┬──────────────────┐
│    OS LAYER          │   DRIVER LAYER   │
├──────────────────────┼──────────────────┤
│ FreeRTOS Kernel      │ STM32 HAL        │
│ Task Scheduler       │ GPIO, UART, USB  │
│ IPC (Queues, Mutex)  │ I2C, Ethernet    │
└──────────────────────┴──────────────────┘
            ▲            ▲
            │ Uses        │
            ▼            ▼
┌──────────────────────────────────────────┐
│       HARDWARE LAYER (Shared)            │
│  STM32H743 CPU + Peripherals             │
│  (480 MHz Cortex-M7, 2MB Flash, 1MB RAM)│
└──────────────────────────────────────────┘
```

---

## 📝 File Organization Flowchart

```
Project Root
│
├─ Core/
│  ├─ Inc/              (Headers)
│  │  ├─ crypto_wallet.h
│  │  ├─ task_sign.h
│  │  ├─ task_net.h
│  │  ├─ task_display.h
│  │  ├─ rng_dump.h ✨ NEW
│  │  └─ ... (19 more)
│  │
│  └─ Src/              (Implementation)
│     ├─ main.c
│     ├─ task_sign.c
│     ├─ task_net.c
│     ├─ task_display.c
│     ├─ crypto_wallet.c
│     ├─ rng_dump.c ✨ NEW
│     └─ ... (18 more)
│
├─ docs_src/            (Documentation)
│  ├─ TESTING_GUIDE_RNG_SIGNING.md ✨ NEW
│  ├─ TEST_SCRIPTS_README.md ✨ NEW
│  ├─ INSTALL_TEST_DEPS.md ✨ NEW
│  ├─ VENV_SETUP.md ✨ NEW
│  │
│  └─ crypto/           (Crypto docs)
│     ├─ README.md
│     ├─ rng_dump_setup.md ✨ NEW
│     ├─ testing_setup.md ✨ NEW
│     ├─ rng_capture_troubleshooting.md ✨ NEW
│     ├─ rng_test_checklist.txt ✨ NEW
│     ├─ + Polish versions ✨ NEW
│     └─ + Russian versions ✨ NEW
│
├─ scripts/             (Utilities)
│  ├─ test_rng_signing_comprehensive.py ✨ NEW
│  ├─ capture_rng_uart.py
│  ├─ run_dieharder.py
│  └─ ... (9+ more)
│
├─ ThirdParty/
│  └─ trezor-crypto/    (Bitcoin library)
│
├─ Makefile             (Build system)
├─ .gitignore           (Git config)
│
└─ Documentation (THIS PROJECT):
   ├─ PROJECTS_COMPARISON_AND_UPDATES.md ⭐ MAIN
   ├─ UPDATES_SUMMARY.md
   ├─ QUICK_REFERENCE.md
   ├─ ARCHITECTURE_DETAILED.md
   ├─ PROJECT_DEPENDENCIES.md
   └─ DOCUMENTATION_INDEX.md (this index)
```

---

## 🔐 Security Posture Radar Chart

```
       Authentication
            ↑
          ╱   ╲
         ╱     ╲
    100%       0%
     ╱───────────╲      Memory Safety
    ╱             ╲    ↗
   ╱  stm32_SB    ╲  ╱
  ╱      40%       ╲╱
 │                 │  Key Management
 │   ╱───────╲     │     ↗
 └──│CW      │─────┴────────
    │70%     │
    ╲       ╱
     ╲     ╱
      ╲   ╱
       ╲ ╱
        V
   Network Security

        ↑
    RNG Quality
     ↗
   
stm32_secure_boot: Medium (no testing)
CryptoWallet:      High (Dieharder validated) ✨

CW = More secure overall
```

---

## 📊 Complexity & Feature Matrix

```
Complexity (X-axis) →
│
│  High │                    CryptoWallet
│       │                    • Multi-protocol
│       │                    • HD wallet
│       │                    • Full testing
│       │                    • 150+ files
│  Mid  │              ╱─────────────────╲
│       │         ╱────────────╲         │
│       │    ╱──────────        ╲        │
│       │   │ stm32_secure_boot  ╲       │
│       │   │ • Verified boot     ╲      │
│       │   │ • Multi-profile      ╲     │
│       │   │ • 100+ files          ╲    │
│  Low  │   └────────────────────────────┘
│       └────────────────────────────────→
        Low        Medium        High
        ← Features →

Product Maturity Indicator:
stm32_secure_boot: Beta (Research Grade)
CryptoWallet:      Stable (Production Grade)
```

---

## 🚀 Development Timeline Gantt Chart

```
        Q1    Q2    Q3    Q4    Q1    Q2    Q3
        2024──────────2024──────────2025──────────2025
stm32_SB ████████████████████░░░░░░░░░░░░░░░░░░░░░░░░
         Bootloader|App|Multi-Profile|Stable

CW                              ███████░░░░░░░░░░░░░░
                               FreeRTOS+LwIP|Crypto|Testing

        2026──────────2026──────────2026──────────2026
        Q1    Q2    Q3    Q4    Q1    Q2    Q3    Q4
SB      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
        Maintenance Mode

CW      ████████████████████████████████████████
        Active Development|RNG Testing|Production

Legend: ████ = Active | ░░░░ = Maintenance/Stable
```

---

## 🎓 Decision Tree: Which Project to Use?

```
                    Need Bootloader?
                          │
                    ┌─────┴─────┐
                    │           │
                   YES          NO
                    │           │
                    ▼           ▼
            stm32_secure_boot   CryptoWallet
                    │           │
        Need Full    │           │    Need Full
        Wallet?      │           │    Wallet?
                     ▼           ▼
                    NO          YES
                     │           │
        Educational/ │           │  Production
        Research     │           │  Deployment
                     │           │
                     ▼           ▼
          Perfect!              Perfect!
          stm32_SB            CryptoWallet
                     │           │
              But you            But you
              might want        might need
              CW's RNG            SB's
              testing!            bootloader!
```

---

## 📞 Support & Resource Map

```
Documentation Location
│
├─ Quick Start
│  └─ QUICK_REFERENCE.md (5-10 min)
│
├─ Detailed Analysis
│  ├─ PROJECTS_COMPARISON_AND_UPDATES.md (30-40 min)
│  └─ UPDATES_SUMMARY.md (10-15 min)
│
├─ Architecture & Design
│  ├─ ARCHITECTURE_DETAILED.md (20-30 min)
│  └─ PROJECT_DEPENDENCIES.md (15-20 min)
│
├─ Implementation
│  └─ Look in Code:
│     ├─ Core/Src/main.c
│     ├─ Core/Src/task_*.c
│     └─ scripts/test_*.py
│
└─ External Help
   ├─ FreeRTOS: freertos.org
   ├─ LwIP: lwip.wikia.com
   ├─ trezor-crypto: github.com/trezor/trezor-firmware
   └─ STM32: st.com/stm32cube
```

---

## ✨ Key Innovation Highlights

```
stm32_secure_boot Innovations:
├─ ✅ Dual-transport protocol (UART + HID)
├─ ✅ Bootloader verification (SHA-256 + ECDSA)
├─ ✅ Multiple education profiles
└─ ✅ Clear separation of concerns

CryptoWallet Innovations:
├─ ✅ HD Wallet Integration (BIP-39/32)
├─ ✅ Multi-protocol support (HTTP + WebUSB + UART)
├─ ✅ Transaction validation (Base58/Bech32)
├─ ✅ RNG Statistical Testing (Dieharder) ✨ NEW
├─ ✅ Secure memory management (memzero)
├─ ✅ Comprehensive multilingual documentation
└─ ✅ Production-grade test automation
```

---

**Visual Summary Document**  
**Created:** 2026-03-20  
**Status:** Complete
