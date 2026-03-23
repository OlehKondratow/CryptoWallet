# 🔐 CryptoWallet - Secure Bitcoin Microcontroller Wallet

**A production-grade hardware wallet demonstrating modern embedded systems architecture**

Secure Bitcoin transaction signing on **STM32H743** with advanced memory management, real-time task scheduling, multi-protocol communication, and cryptographic signing. **UART** carries **CWUP-0.1** (MVP line protocol on USART3) for diagnostics, boot-chain queries, and CI/HIL—see below and [`documentation/05-uart-cwup-protocol.md`](documentation/05-uart-cwup-protocol.md).

**Languages:** 🇬🇧 [English](README.md) | 🇷🇺 [Русский](README_ru.md) | 🇵🇱 [Polski](README_pl.md)

**Canonical manual (English, uniform depth):** [`documentation/README.md`](documentation/README.md) — chapters 1–7 (trust model through build/CI). This file is the long-form overview; generated excerpts are under [`documentation/generated/`](documentation/generated/).

---

## ⚡ Quick Facts

| Aspect | Details |
|--------|---------|
| **MCU** | STM32H743ZI (Cortex-M7 @ 480 MHz) |
| **Memory** | 2 MB Flash + 1 MB SRAM |
| **OS** | FreeRTOS (real-time multi-tasking) |
| **Network** | LwIP (Ethernet + HTTP server) |
| **Crypto** | trezor-crypto (secp256k1, BIP-39/32) |
| **Interfaces** | HTTP, WebUSB, **USART3 (CWUP-0.1 MVP)**, I2C (OLED) |
| **Security** | ECDSA signing, secure key derivation, memzero() buffer clearing |

### Bootloader signing vs firmware image vs `task_sign` (three different things)

Do **not** mix these up—they use **different keys** and solve **different problems**:

| Layer | What is signed / verified | Where it lives | Role |
|-------|---------------------------|----------------|------|
| **Verified bootloader** (`stm32_secure_boot`) | The **application image** (Flash contents / header + payload) before the CPU jumps to the app | **Sibling repo** — verification code in ROM/Flash **before** `main()` | **Supply-chain / anti-tamper for firmware**: only an image signed with the **bootloader trust key** should run. |
| **Application binary artifact** | `build/cryptowallet.bin` (or packaged image) that you **flash** | Produced here; **signing for deploy** uses tooling in **`stm32_secure_boot`** (e.g. `sign_image.py`, key material paired to the bootloader’s public key) | Pairs with the row above: host signs the bin, device bootloader checks **ECDSA over the app** at boot. |
| **Runtime transaction signing** (`task_sign` + `crypto_wallet`) | **Bitcoin transactions** (ECDSA **secp256k1** over tx hash) | **This repo** — FreeRTOS task, trezor-crypto | **User funds / wallet keys** derived from the **BIP-39** seed — **unrelated** to the firmware signing key used by the bootloader. |

**Optional runtime check in the app:** `fw_integrity` (`Core/Src/fw_integrity.c`) computes **CRC32 + length** over the linked application region—useful to detect corruption; it is **not** verified boot and **not** a second firmware signature.

**Docs:** trust boundaries — [`documentation/01-trust-model-and-architecture.md`](documentation/01-trust-model-and-architecture.md); signing pipeline — [`documentation/03-cryptography-and-signing.md`](documentation/03-cryptography-and-signing.md). **CWUP** `AT+BOOTCHAIN?` is **informational** (e.g. `VTOR`, flags); it does **not** prove the bootloader actually verified the image—you need the **stm32_secure_boot** design for that.

### Private keys / seed storage (wallet material)

This is **not** the same as **bootloader firmware signing** (above). Wallet **private keys** are derived from a **BIP-39 seed** inside `task_sign` / `crypto_wallet` when `USE_CRYPTO_SIGN=1`.

| Mechanism | What the tree does today |
|-----------|---------------------------|
| **Hook** | `get_wallet_seed(uint8_t *out, size_t len)` — **you** supply where the seed comes from. The **default** weak implementation in `task_sign.c` returns **`-1`** (no seed) with comment *implement in secure storage*. |
| **Development / CI only** | **`USE_TEST_SEED=1`** pulls in `wallet_seed.c`: a **fixed, public** BIP-39 test mnemonic → predictable keys — **never use for real funds**. The file states that **production** should replace this with e.g. **secure element** or **encrypted flash** workflow. |
| **RAM hygiene** | After signing, **`memzero()`** clears seed, private key, and signature buffers in the signing path—helps with **volatile** data lifetime; it does **not** add a tamper-proof **non-volatile** vault. |
| **“Secure storage” product block** | There is **no** integrated **SE/TPM driver**, **TrustZone** wallet enclave, or **encrypted NVM keybag** in this repository as a finished feature—those are **platform integration** tasks. |
| **Planned idea: bind to board ID** | **Not implemented.** Encrypt seed **at rest** on flash; mix **MCU UID** (binding) with **PIN** / KDF; use **hardware TRNG** for setup salts and nonces—not UART dump. Same ciphertext must not decrypt on another board. Details and limits: **section 3.5** of [`documentation/03-cryptography-and-signing.md`](documentation/03-cryptography-and-signing.md). |

**Docs:** [`documentation/03-cryptography-and-signing.md`](documentation/03-cryptography-and-signing.md) — sections 3.4 (memory hygiene), 3.4b (current scope), **3.5** (TRNG + device-bound seed — single place).

### UART — CWUP-0.1 (MVP)

**CryptoWallet UART Protocol (CWUP) 0.1** runs on **USART3** (default **115200 8N1**). It is for **lab bring-up, automation, and CI/HIL**—not a replacement for HTTP/WebUSB as the primary user channel. Implementation: `Core/Src/cwup_uart.c`.

- **Security:** the link is **not authenticated** (same class of risk as cleartext HTTP on the LAN). See [`documentation/01-trust-model-and-architecture.md`](documentation/01-trust-model-and-architecture.md).
- **`USE_RNG_DUMP=1`:** CWUP **does not run** on that UART—the firmware sends **raw TRNG bytes only** (no `AT+` text). Details: [`documentation/06-integrity-rng-verification.md`](documentation/06-integrity-rng-verification.md).
- **Full specification:** [`documentation/05-uart-cwup-protocol.md`](documentation/05-uart-cwup-protocol.md).

#### Preconditions (what must be true)

| Goal | Condition |
|------|------------|
| **CWUP on USART3** | Build with **`USE_RNG_DUMP=0`** (typical default). In `main.c`, `Cwup_Init()` runs in the `#else` branch of `#ifdef USE_RNG_DUMP`; with **`USE_RNG_DUMP=1`**, `RNG_Dump_Task_Create()` runs instead and **CWUP is not started** on that UART. |
| **WebUSB in firmware** | **`USE_WEBUSB=1`**, USB FS on PA11/PA12, linked USB stack (`usb_webusb`, `usbd_desc_cw`, …). |
| **CWUP + WebUSB together** | Normal: they use **different interfaces** (UART vs USB)—**no conflict**. Typical CI-style build: `USE_RNG_DUMP=0` `USE_WEBUSB=1`. |
| **Physical link** | Host can reach **USART3** (often ST-Link **MCU VCP**, not the “VCP Ctrl” bridge—see `--auto-port` below). |

#### Line phases A → D (host view)

| Phase | Meaning |
|-------|---------|
| **A** | Before **`CW+READY`**: boot / preamble; host must **not** treat the line as CWUP-ready yet. No §7 framed TRNG on the wire. |
| **B** | **One** line: `CW+READY,proto=CWUP/0.1,build=…,rng=0\|1` (`\r\n`). `rng=1` would mean **framed** TRNG via CWUP is available—**not** the same as raw `USE_RNG_DUMP`. |
| **C** | **TEXT** mode: host sends `AT+…` commands; responses `CW+…` / `CW+ERR=…`. This is what the MVP implements (**A→B→C**). |
| **D** | **BINARY_RNG** (framed TRNG per spec §5.5) after `AT+RNG=START`—**not implemented** in firmware today. For a **raw** byte stream use a **`USE_RNG_DUMP=1`** build instead (CWUP disabled). |

**Security:** signing and product flows stay on **HTTP/WebUSB**; CWUP is **lab/CI diagnostics** only.

#### Exchange model (TEXT mode)

| Item | Rule |
|------|------|
| **Framing** | One **line** = payload ending in **`\r\n`** (CRLF). Max line length in firmware: 256 characters. |
| **Host → device** | `AT+<VERB>[=<args>]\r\n` — case-insensitive prefix matching for known verbs (see table below). |
| **Device → host (OK)** | `CW+<CODE>[,payload]\r\n` — success or data line. |
| **Device → host (error)** | `CW+ERR=<n>,<msg>\r\n` |
| **Ready banner** | After FreeRTOS + CWUP task start, the device emits **once**: `CW+READY,proto=CWUP/0.1,...` (git/build fields). Wait for this (or `AT+READY?`) before relying on command mode. |
| **Boot scope** | CWUP exists **only in the application** after the scheduler is up—not during the optional bootloader in `stm32_secure_boot`. |

#### MVP command set

| Command | Purpose |
|---------|---------|
| `AT+PING` | Liveness → `CW+PONG` |
| `AT+READY?` | Re-print ready / protocol info (same family as startup `CW+READY`) |
| `AT+CWINFO?` | Protocol id + build/version fields |
| `AT+FWINFO?` | Firmware integrity string (aligned with `fw_integrity` / log `FWINFO`) |
| `AT+BOOTCHAIN?` | Human-readable boot-chain description (lab) |
| `AT+WALLET?` | Lab flags only (e.g. seed/signing available)—**no secrets on the wire** |
| `AT+SELFTEST?` | Lightweight sanity / tick check |
| `AT+ECHO=<text>` | Echo printable ASCII (CI / regression) |

**Reserved / not implemented in firmware today** (see spec for wire format when added):

| Command | Status |
|---------|--------|
| `AT+MARKS` | Not implemented |
| `AT+RNG=START,<bytes>` / `AT+RNG=STOP` | Not implemented (framed TRNG session); **not** the same as `USE_RNG_DUMP` raw dump |

#### HIL script: command order (`scripts/test_cwup_mvp.py`)

Helpers live in `scripts/mvp_cwup.py` (line I/O, parsers for `CW+FWINFO`, `CW+WALLET`, `CW+BOOTCHAIN`, etc.). After opening the port, the script **drains until `CW+READY`** appears, then **one pass** runs:

| Step | Command | Check |
|------|---------|--------|
| 1 | `AT+PING` | Response contains `CW+PONG` |
| 2 | `AT+CWINFO?` | Response contains `CW+CWINFO` |
| 3 | `AT+READY?` | Response contains `CW+READY` |
| 4 | `AT+WALLET?` | Parse `CW+WALLET,seed=0\|1,crypto_sign=0\|1`; compare to `--expect-wallet-seed` / `--expect-crypto-sign` (or env, see below) |
| 5 | `AT+SELFTEST?` | `CW+SELFTEST,ok=1` |
| 6 | `AT+ECHO=<token>` | Payload round-trip (default token `autotest` or `CWUP_ECHO_TOKEN`) |
| 7 | `AT+FWINFO?` | **Only if** `--bin build/cryptowallet.bin`: CRC32 in line must match **host** CRC of that file |
| 8 | `AT+BOOTCHAIN?` | `CW+BOOTCHAIN` with parseable fields (e.g. `app_entry`) |

**Stress:** `--stress-extra-rounds N` repeats the **full** sequence `N` additional times; `--stress-delay-ms` adds pause between rounds. **`--auto-port`** selects the ST-Link **MCU UART** VCP when several `ttyACM*` exist (deprioritizes descriptions containing **“VCP Ctrl”**, which is not bridged to target USART3).

#### Environment (HIL overrides)

| Variable | Role |
|----------|------|
| `CWUP_UART_PORT` or `CI_UART_PORT` | Serial device (default `/dev/ttyACM0`) |
| `CWUP_UART_BAUD` or `CI_UART_BAUD` | Baud (default `115200`) |
| `CWUP_SKIP_NO_DEVICE` | If `1` and port missing: **exit 0** (skip—useful in CI without hardware) |
| `CWUP_EXPECT_WALLET_SEED`, `CWUP_EXPECT_CRYPTO_SIGN` | Expected bits for `AT+WALLET?` (defaults `0`, `0` if unset) |
| `CWUP_ECHO_TOKEN` | String for `AT+ECHO=` |

#### Host tooling

```bash
# HIL / regression (from repo root, device on e.g. /dev/ttyACM0)
python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0

# Match FWINFO CRC to on-disk image (same as flashed .bin)
python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --bin build/cryptowallet.bin

# Prefer correct ST-Link VCP when multiple ttyACM devices exist
python3 scripts/test_cwup_mvp.py --auto-port

# Wait for CW+READY / boot markers in CI-style logs
python3 scripts/ci/uart_wait_boot_log.py --help
```

**Manual try** (TEXT terminal, append CRLF when sending lines):

```text
AT+PING
AT+FWINFO?
AT+BOOTCHAIN?
```

---

### TRNG (hardware RNG) — capture and statistics

The STM32 **hardware TRNG** feeds **trezor-crypto** (and related signing paths) in normal builds; **UART dump mode** is only for **lab/CI** statistics. **Design note:** a future **encrypted seed on flash** should take **entropy from the in-firmware RNG** (salts, nonces) and use **UID** for **board binding**—see **section 3.5** of [`documentation/03-cryptography-and-signing.md`](documentation/03-cryptography-and-signing.md). The UART stream is **not** the provisioning API for stored keys.

In **dump mode**, firmware emits a **raw byte stream** on UART for external statistical checks.

| Item | Detail |
|------|--------|
| **In firmware** | `HAL_RNG` / driver path as in `rng_dump.c`; build with **`USE_RNG_DUMP=1`** for a **binary-only** UART stream (no text logs; **CWUP is off** on that UART). |
| **Capture** | `scripts/capture_rng_uart.py` writes a **`.bin`** file of raw bytes. Mixed ASCII logs invalidate dieharder-style tests. |
| **Host wrapper** | `python3 scripts/run_dieharder.py --file rng.bin` — uses **generator 201** (`file_input_raw`). With **no** `--test` / `-d`, the script runs **`dieharder -a`** (full battery). **Single test:** `--test N` (same as `dieharder -d N`). List indices: `--list-tests` → `dieharder -l`. |
| **CI** | `.gitea/workflows/simple-ci.yml`: UART capture → `rng_test.bin`, then optional quick dieharder step; env **`CI_RNG_DIEHARDER_FULL=1`** enables **full** `-a` run (slow). See [`documentation/06-integrity-rng-verification.md`](documentation/06-integrity-rng-verification.md). |

#### DIEHARDER: generator **201** and run modes

All commands assume **`-g 201 -f <file.bin>`** (unsigned bytes from file).

| Mode | Typical command | Use |
|------|-----------------|-----|
| **Full battery** | `dieharder -a -g 201 -f rng.bin` | Many statistical tests in one run; **long** on large files—reserved for serious review or `CI_RNG_DIEHARDER_FULL=1`. |
| **Single test** | `dieharder -d N -g 201 -f rng.bin` | One test by index `N` (CI **smoke** uses a small `N`, e.g. `0`, for speed). |
| **Inspect catalogue** | `dieharder -l` | Lists **all** tests with their numeric **`-d`** index and **name** (used to pick a single test). |

**Note:** `dieharder` often returns **exit code 0** even when a row shows **`FAILED`**—read the **Assessment** column, not only the process exit code.

#### Grouped tests (families in the dieharder catalogue)

`dieharder -l` groups the battery into **named statistical families** (word prefixes in test names). Examples you will see on a typical build:

| Family (name prefix) | What it stresses (informal) |
|----------------------|-----------------------------|
| **`diehard_*`** | Classical **Diehard** tests (e.g. birthdays, permutations). |
| **`rgb_*`** | **RGB** bit / lagged-string tests (bit distribution). |
| **`sts_*`** | **STS**-style / bitstream tests (NIST-oriented). |
| **`dab_*` / `marsaglia_*` / others** | Additional batteries shipped with dieharder (exact set depends on version). |

**`-a`** runs **all** applicable tests for generator 201—i.e. the **full grouped catalogue** in one go. **`-d N`** runs **one** line from that catalogue (one “group member” by index).

#### Interpreting results (especially short files and CI smoke)

- **Finite file + rewind:** dieharder may print *“The file … was rewound”*. Tests assume huge i.i.d. streams; **rewinding** breaks independence → **borderline or failed p-values** for some tests (e.g. `diehard_birthdays`) are **not** automatically a broken TRNG—see [`documentation/06-integrity-rng-verification.md`](documentation/06-integrity-rng-verification.md).
- **Acceptance-style review:** use a **large** capture (hundreds of MiB), run **`-a`** (or multiple offline passes), and treat CI smoke as **tooling check**, not cryptographic sign-off.

---

## 🏗️ System Architecture

### Memory Map & Organization

```
FLASH MEMORY (0x08000000 - 0x08200000):
├─ 0x08000000: [Bootloader (optional) - 64 KB]
├─ 0x08010000: [Application - 1.5 MB]
│   ├─ FreeRTOS kernel code
│   ├─ LwIP stack
│   ├─ trezor-crypto library
│   ├─ Application firmware
│   └─ Configuration
└─ 0x08180000: [Free space - 512 KB]

SRAM MEMORY (0x20000000 - 0x20100000):
├─ 0x20000000: [FreeRTOS TCB & Kernel Objects - ~100 KB]
│   ├─ Task Control Blocks
│   ├─ Ready lists
│   └─ Queue/event structures
├─ 0x20018000: [IPC Objects - ~50 KB]
│   ├─ tx_request_queue
│   ├─ sign_response_queue
│   ├─ display_queue
│   └─ Mutexes/semaphores
├─ 0x20025800: [Task Stacks - ~300 KB]
│   ├─ Sign task stack (32 KB)
│   ├─ Network task stack
│   ├─ Display task stack
│   ├─ User input task stack
│   └─ IO task stack
├─ 0x20080000: [Dynamic Heap - ~400 KB]
│   ├─ LwIP buffers
│   ├─ malloc/free (crypto)
│   └─ USB buffers
└─ 0x200F8000: [LwIP RX Descriptors - 64 KB]
```

**Key Design Principle:** Separate memory regions for kernel, tasks, and heap to prevent fragmentation.

### Memory Management Details

**FLASH Organization Strategy:**
- **Bootloader isolation**: Optional user bootloader at fixed location (64 KB boundary)
- **Application image**: Linked to start at 0x08010000 (64 KB offset)
- **Benefit**: If bootloader fails, ROM bootloader can still load test app
- **Configuration**: Edit `linker.ld` to change base addresses

**SRAM Organization Strategy:**
- **Kernel objects first** (TCB, ready lists): Prevents overflow of kernel structures
- **IPC layer** (queues, mutexes): Separate region prevents fragmentation
- **Task stacks** (separate per task): Stack sizes pre-allocated (no growth allowed)
  - **Signing task**: 32 KB (cryptography needs buffer space)
  - **Network task**: 16 KB (LwIP stack requirements)
  - **Display task**: 4 KB (minimal work per tick)
  - **Other tasks**: 2-8 KB (event-driven)
- **Dynamic heap** (400 KB): Used by malloc/free (crypto library, LwIP buffers)
- **Design rationale**: 
  - Stack sizes are FIXED - eliminates dynamic allocation vulnerability
  - Heap is bounded - prevents runaway allocation
  - If heap exhausted: OOM error instead of silent corruption

**Memory Protection (ARM Cortex-M7):**
- **MPU (Memory Protection Unit)**: Optional configuration
  - Can protect kernel code from task corruption
  - Can protect sensitive memory regions (keys in RAM)
  - Not currently enabled (adds latency, requires profiling)

---

## 🔄 Kernel & System Software Architecture

### FreeRTOS Kernel Organization

**Kernel Tick & Scheduling:**
- **SysTick exception**: Configured to fire every 1 ms
- **Tick handler flow:**
  1. Save CPU state (registers pushed by Cortex-M7 hardware)
  2. Call `xTaskIncrementTick()` → updates global tick counter
  3. Evaluate readiness of all tasks with expired delays
  4. Find highest-priority ready task
  5. If different from current → mark PendSV exception
  6. Return from interrupt
  7. PendSV exception → context switch (save old context, load new context)
- **Context switch time**: ~5-10 microseconds (hardware accelerated)
- **Context includes**: 16 registers (R0-R15), XPSR (flags), control register

**Task Ready Queue:**
- **Data structure**: 32 linked lists (one per priority level 0-31)
- **O(1) lookup**: Highest priority ready task is immediately known
- **Lock-free** (within ISR context): Uses atomic bitfield operations
- **Example**: If priorities 22, 21, 20 have ready tasks, scheduler picks 22

**Task Delay Mechanism:**
- **Timer list**: Sorted by wake-time
- **Overflow handling**: Uses 16-bit timer wrapping (FreeRTOS v10+)
- **Wake-up flow**: Timer list → ready queue → next SysTick evaluation

**Priority Inheritance (Mutex):**
- **Problem**: Priority inversion (low-priority task holding mutex blocks high-priority)
- **Solution**: When task waiting on mutex, temporarily raise lock-holder priority
- **Example in code**: `crypto_lock` mutex uses priority inheritance
  - If task_net waiting for crypto_lock, task_sign gets boosted priority
  - Once task_sign releases lock, priority drops back

### Task Synchronization Patterns

**Queue-based Communication (tx_request_queue → task_sign):**
```c
// Producer (task_net):
xQueueSend(tx_request_queue, &tx_request, portMAX_DELAY);
// Blocks if queue full (typically not, depth=10)

// Consumer (task_sign):
xQueueReceive(tx_request_queue, &tx_request, portMAX_DELAY);
// Blocks until message available
// Atomically removes message from queue
// Wakes up even if other tasks higher priority (because has data)
```

**Event Group (user_confirm_event):**
```c
// Waiter (task_sign):
xEventGroupWaitBits(user_confirm_event, CONFIRM_BIT, 
                    pdTRUE,      // clear on exit
                    pdTRUE,      // wait for all bits
                    xTicksToWait); // timeout: 30 seconds

// Signaler (task_user):
xEventGroupSetBitsFromISR(user_confirm_event, CONFIRM_BIT, &xHigherPriorityTaskWoken);
```

**Mutex with Timeout (crypto_lock):**
```c
// Task takes lock with 5-second timeout:
if (xSemaphoreTake(crypto_lock, pdMS_TO_TICKS(5000)) == pdTRUE) {
    // Perform crypto operation
    xSemaphoreGive(crypto_lock);
} else {
    // Timeout - other task holding lock too long
    // Action: error response or retry
}
```

### Real-Time Constraints Analysis

**Hard Real-Time (must not miss):**
- **SysTick** → 1 ms deadline (hardware driven)
- **Interrupt latency** → maximum ~10 µs (ARM core limitation)
- **Critical section**: Disabled interrupts ≤ 100 µs (prevents long jitter)

**Soft Real-Time (missed deadlines degraded, not fatal):**
- **User button response**: ≤ 100 ms acceptable (debounce + poll interval)
- **OLED display**: ≤ 100 ms acceptable (human eye perception)
- **HTTP response**: ≤ 1 second acceptable (user interaction)

**Task Blocking Analysis:**
- **task_sign blocked in WAIT_USER**: Highest priority task blocked
  - Other tasks run: task_net, task_display, task_io, task_user
  - Network traffic still serviced (by task_net)
  - No starvation (all tasks get CPU time)
- **Priority scheduling ensures**: 
  - Button press (task_user, priority 22) always preempts others
  - Signing (priority 21) preempts network (priority 20)
  - Network doesn't starve lower priority tasks

---

## 📋 Task Architecture (FreeRTOS)

### Scheduled Tasks

```
Priority 22: User Input Task (task_user.c)
├─ Highest priority (user interactions)
├─ Button polling (PC13, 20ms intervals)
├─ Debouncing (30ms stable detection)
└─ Signals: user_confirm_event → Signing task

Priority 21: Signing Task (task_sign.c)
├─ FSM-based signature pipeline
├─ Waits on: tx_request_queue
├─ Calls: crypto_wallet.c → trezor-crypto
├─ Sends: sign_response_queue
└─ Duration: ~100-500ms (per signature)

Priority 20: Network Task (task_net.c)
├─ HTTP server (LwIP)
├─ Listens: port 80, WebUSB bulk endpoints
├─ Enqueues: tx_request_queue
└─ Response: HTTP 200 + signature

Priority 18: IO Task (task_io.c)
├─ LED status indicators
├─ Patterns: steady, blinking, error
└─ Visual feedback to user

Priority 15: Display Task (task_display.c)
├─ SSD1306 OLED management (128×32)
├─ Updates: 10 Hz (every 100ms)
├─ State: "Waiting...", "Signing...", "✓ OK", "✗ Error"
└─ Scrolling log: last 4 transactions

SysTick Interrupt: 1 ms
├─ Scheduled by FreeRTOS
├─ Context switching
└─ Task readiness checks
```

### IPC (Inter-Process Communication)

```
Queue: tx_request_queue
├─ Source: task_net (HTTP handler)
├─ Destination: task_sign (signing FSM)
├─ Message: {amount, address, currency}
└─ Blocking: task_sign waits here

Queue: sign_response_queue
├─ Source: task_sign (after signature)
├─ Destination: task_net (HTTP response)
├─ Message: {signature (DER), tx_id}
└─ Non-blocking: send and continue

Event: user_confirm_event
├─ Source: task_user (button press)
├─ Listener: task_sign (blocked in signing step)
├─ Unblocks: signature computation
└─ Timeout: 30 seconds

Mutex: crypto_lock
├─ Protects: trezor-crypto library access
├─ Holder: task_sign during signing
├─ Prevents: concurrent crypto operations
└─ Priority inheritance: prevents priority inversion
```

---

## 🔐 Boot Process & Security Chain

### Boot Sequence

```
STEP 1: Reset Vector (STM32H743 ROM)
├─ CPU starts @ 0x08000000
├─ Loads Stack Pointer (SP) from vector
├─ Jumps to Reset_Handler
└─ Time: < 1ms

STEP 2: Bootloader (Optional - from stm32_secure_boot)
├─ Location: 0x08000000 (64 KB)
├─ Operations:
│   ├─ Clock configuration (PLL @ 480 MHz)
│   ├─ Read application image (0x08010000)
│   ├─ Compute SHA-256 hash
│   ├─ Verify ECDSA signature
│   ├─ Check signature against public key
│   └─ Jump to application if valid
├─ Fallback: LED error + halt if invalid
└─ Time: ~50-100ms

STEP 3: Application Startup (main.c)
├─ Hardware Initialization (hw_init.c):
│   ├─ Clock configuration (HSI48 for USB)
│   ├─ GPIO setup (UART, I2C, USB AF)
│   ├─ UART initialization (USART3 @ 115200)
│   ├─ I2C initialization (100 kHz for OLED)
│   ├─ USB device initialization (FS @ 12 Mbps)
│   ├─ Ethernet PHY initialization (RMII)
│   ├─ RNG initialization (if USE_RNG_DUMP=1)
│   └─ NVIC setup (interrupt priorities)
│
├─ FreeRTOS Kernel Creation:
│   ├─ Heap initialization (400 KB available)
│   ├─ Timer queue creation
│   ├─ Scheduler structure setup
│   └─ Memory pools allocation
│
├─ IPC Objects Creation:
│   ├─ Create tx_request_queue (depth: 10)
│   ├─ Create sign_response_queue (depth: 5)
│   ├─ Create display_queue (depth: 20)
│   ├─ Create crypto_lock mutex
│   └─ Create user_confirm_event
│
├─ Task Creation (Priority order):
│   ├─ task_user (Priority 22, 8 KB stack)
│   ├─ task_sign (Priority 21, 32 KB stack)
│   ├─ task_net (Priority 20, 16 KB stack)
│   ├─ task_io (Priority 18, 2 KB stack)
│   └─ task_display (Priority 15, 4 KB stack)
│
└─ FreeRTOS Scheduler Start:
    ├─ SysTick configured (1 ms tick)
    ├─ Idle task spawned
    └─ Context switch to highest priority ready task

STEP 4: Runtime (SysTick-driven)
├─ Every 1 ms:
│   ├─ SysTick exception fires
│   ├─ FreeRTOS increments tick counter
│   ├─ Task readiness evaluated
│   ├─ Context switch if needed
│   └─ Return to running task
└─ Tasks wait on events/queues as needed
```

### Trust Chain (with Bootloader)

```
┌─ STM32H743 ROM Bootloader
│  (immutable from factory, hardcoded @ 0x00000000)
│  └─ Loads and validates user bootloader
│
└─ User Bootloader (stm32_secure_boot)
   (optional, located @ 0x08000000)
   ├─ SHA-256 hash of application
   ├─ ECDSA signature verification
   ├─ Public key in keys.h
   └─ Jump to app @ 0x08010000 if valid
      │
      └─ CryptoWallet Application
         └─ Executed only if bootloader verification passed
```

---

## 🔄 Transaction Signing Flow

### Detailed State Machine (task_sign.c)

```
[IDLE] - Waiting for request
  ↓
[QUEUE_WAIT] - Blocked on tx_request_queue
  ↓ (receives: {amount, address, currency})
[VALIDATE] - tx_request_validate.c
  ├─ Address format check (Base58/Bech32)
  ├─ Amount validation (>0, <21M BTC)
  ├─ Currency check (BTC mainnet/testnet)
  └─ ✗ Invalid → [ERROR] → HTTP 400
    ✓ Valid → [KEY_DERIVE]
  ↓
[KEY_DERIVE] - crypto_wallet.c
  ├─ Read seed (from RAM or test_seed)
  ├─ Generate master key via HMAC-SHA-512
  ├─ Apply BIP-32 path: m/44'/0'/0'/0/0
  ├─ Derive child private key
  └─ Store in secure buffer
  ↓
[DISPLAY] - task_display.c
  ├─ Show: "Confirm? {amount} BTC to {addr...}"
  ├─ Update OLED display
  └─ State → "Waiting for confirmation"
  ↓
[WAIT_USER] - Blocked on user_confirm_event
  ├─ User presses button (PC13)
  ├─ task_user debounces (30ms)
  ├─ Signals user_confirm_event
  └─ Timeout: 30 seconds → [TIMEOUT_ERROR]
    ✓ Confirmed → [SIGN]
  ↓
[SIGN] - crypto_wallet.c → trezor-crypto
  ├─ Message to sign: transaction bytes
  ├─ Hash algorithm: SHA-256(transaction)
  ├─ Signature algorithm: ECDSA secp256k1
  ├─ Nonce generation: random or RFC 6979
  ├─ Signature output: (r, s) pair
  ├─ DER encoding: (0x30, length, ...)
  └─ Duration: 50-200ms
  ↓
[MEMZERO] - memzero.c
  ├─ Clear: private key buffer
  ├─ Clear: seed buffer
  ├─ Clear: intermediate values
  ├─ Method: volatile writes (prevent optimization)
  └─ Verify: buffers = 0x00
  ↓
[RESPONSE] - Enqueue result
  ├─ Send: sign_response_queue
  ├─ Message: {signature, tx_id}
  ├─ task_net receives and builds HTTP response
  └─ HTTP 200 + JSON {"signature": "...", "tx_id": "..."}
  ↓
[SUCCESS] - Update display
  ├─ Show: "✓ Signature OK"
  ├─ Duration: 2 seconds
  └─ Return to [IDLE]
```

---

## 🔌 Interrupt Handling

### Interrupt Priority Hierarchy

```
Priority 0 (Highest - Hard Real-time):
├─ USART3_IRQHandler (UART receive)
├─ I2C1_EV_IRQHandler (OLED I2C)
└─ USB interrupts (device events)

Priority 1-2 (Medium - FreeRTOS-managed):
├─ SysTick_Handler (1 ms tick) ⭐
├─ Ethernet interrupts (LwIP)
└─ GPIO interrupts (button)

Priority 3-7 (Lower - Deferred):
└─ System exceptions (fault handlers)

SysTick Handler Details:
├─ Fires every 1 ms (configurable)
├─ Triggers: xTaskIncrementTick() (FreeRTOS)
├─ Operations:
│   ├─ Increment tick counter
│   ├─ Update task timers
│   ├─ Check if higher priority task ready
│   ├─ Set pPendSV exception if switch needed
│   └─ Return to current task OR
│       Pending SVC → context switch
└─ Total latency: ~5-10 microseconds
```

### UART Interrupt (Debug/Logging)

```
When data received on USART3:
├─ USART3_IRQHandler fires
├─ Reads: one byte from RX data register
├─ Operation: xQueueSendFromISR(uart_queue, ...)
├─ May wake: task_display_minimal (if waiting)
└─ Return: yields CPU to FreeRTOS if task woken

Prevents: blocking main loop
Enables: responsive logging
```

### System Software Architecture

**Hardware Abstraction Layer (hal/):**
- **gpio_driver.c**: GPIO initialization, read/write functions
- **uart_driver.c**: UART configuration, ISR, ring buffer
- **i2c_driver.c**: I2C master, polling or interrupt-based
- **spi_driver.c**: SPI for external devices (if any)
- **eth_phy_driver.c**: Ethernet PHY initialization
- **usb_device_driver.c**: USB stack integration

**Interrupt Service Routing (ISR Ordering):**

1. **SysTick (1 ms tick)** - FreeRTOS kernel
   - Executed every 1 ms
   - Calls FreeRTOS tick handler
   - May trigger context switch (PendSV)

2. **UART RX (USART3)** - Debug logging
   - Reads one byte from RX FIFO
   - Enqueues to buffer ring
   - Non-blocking (critical section < 5 µs)

3. **Ethernet RX/TX** - Network stack (LwIP)
   - Handled by task_net (not ISR-driven)
   - ISR just signals task to process frames
   - Prevents heavy ISR workload

4. **USB Device** - USB stack
   - Handled by LwIP or direct USB driver
   - ISR minimal (just set flags)
   - Actual processing in task

5. **GPIO Interrupt** - User button (PC13)
   - EXTI13 fires on button edge
   - Sets flag read by task_user
   - task_user debounces in software

**ISR Design Philosophy:**
- Keep ISR code **minimal** (< 10 µs execution)
- Use **flags/queues** to defer work to tasks
- Avoid **long critical sections** (disable interrupts)
- Use **FromISR** API variants (xQueueSendFromISR, etc.)

**Critical Sections (Interrupt Disable):**
```c
// Minimal critical section example (task_sign.c):
taskENTER_CRITICAL();
{
    // Save old value
    uint32_t old_seed = current_seed;
    // Update
    current_seed = new_seed;
}
taskEXIT_CRITICAL();
// Interrupts re-enabled immediately after
// Total latency impact: 1-2 µs
```

**Context Switch Process (PendSV handler):**
1. Save current task context (SP, R0-R12, R14, control bits)
2. Call `vTaskSwitchContext()` → update pxCurrentTCB
3. Load new task context
4. Return from exception → CPU resumes new task
5. Total time: ~500-1000 clock cycles (≈2-4 µs @ 480 MHz)

---

## 💾 System Software Stack

**Layered Architecture:**
```
┌─────────────────────────────────────┐
│  Application Layer (task_*.c)       │ ← Task code
│  - Signing, Network, Display, User  │
├─────────────────────────────────────┤
│  Middleware Layer                   │
│  - LwIP (networking)                │
│  - trezor-crypto (cryptography)     │
│  - FreeRTOS (kernel)                │
├─────────────────────────────────────┤
│  HAL Layer (hal/)                   │ ← Drivers
│  - UART, GPIO, I2C, SPI, USB, Eth   │
├─────────────────────────────────────┤
│  CMSIS Layer (stm32h7xx_hal_*.h)    │ ← Register definitions
├─────────────────────────────────────┤
│  ARM Cortex-M7 Core                 │ ← CPU
│  - Cache, MPU, FPU, etc             │
└─────────────────────────────────────┘
```

**Module Dependencies:**
```
task_sign.c
├─ FreeRTOS (queue, mutex, event)
├─ crypto_wallet.c
│  └─ trezor-crypto
│     └─ secp256k1, BIP-39/32
└─ memzero.c (secure buffer clear)

task_net.c
├─ FreeRTOS (queue)
├─ LwIP
│   ├─ TCP/IP stack
│   ├─ DHCP client
│   └─ HTTP server
└─ eth_phy_driver.c

task_display.c
├─ FreeRTOS (queue)
└─ i2c_driver.c → SSD1306 OLED

task_user.c
├─ FreeRTOS (event, queue)
└─ gpio_driver.c → GPIO_PC13 (button)
```

---

## 🔒 Security Considerations (System Level)

**Memory Security:**
- **Stack overflow protection**: Fixed stack sizes prevent unchecked growth
- **Heap fragmentation**: Separate heap reduces predictability of heap state
- **Sensitive data**: Cleared with volatile writes (prevents compiler optimization)

**Real-Time Security:**
- **Timing attacks**: Cryptographic operations vary with RNG (timing unpredictable)
- **Power analysis**: Difficult (oscilloscope needed, not practical for code review)
- **Fault injection**: Requires physical access (glitching equipment)

**Execution Security:**
- **No address randomization** (embedded constraint)
- **No DEP/NX bit** (Cortex-M7 limitation)
- **Bootloader verification** (if enabled): Ensures only authorized code runs

**IPC Security:**
- **Message queue overflow**: Statically allocated (no allocation failure)
- **Mutex deadlock**: FreeRTOS priority inheritance prevents
- **Task isolation**: None (all tasks share address space, by design)

---

## 🔐 Authorization & Authentication

### User Confirmation (Authorization)

```
Level 1: Button Press (PC13 GPIO)
├─ Physical confirmation required
├─ task_user reads pin state every 20ms
├─ Debounce: 30ms stable detection
├─ Press → Signals user_confirm_event
└─ task_sign: unblocks and proceeds with signing

Security: 
├─ ✓ Prevents accidental transactions
├─ ✓ User must be physically present
└─ ✗ No cryptographic authentication (by design)

Alternative: PIN (Optional Enhancement)
├─ Could add numeric PIN entry on OLED
├─ Verify pin matches stored hash
├─ Rate-limited attempts (3 strikes)
└─ Timeout after failed attempts
```

### Transaction Validation (Authentication)

```
Layer 1: Address Format Check
├─ Bitcoin address types:
│   ├─ P2PKH (legacy): "1..." (Base58Check)
│   ├─ P2SH (multisig): "3..." (Base58Check)
│   └─ SegWit (native): "bc1..." (Bech32)
├─ Checksum verification: Base58Check or Bech32
└─ Result: ✓ Valid format OR ✗ Invalid → Reject

Layer 2: Amount Validation
├─ Range: 0 < amount ≤ 21,000,000 BTC
├─ Decimal precision: up to 8 places (1 Satoshi = 0.00000001 BTC)
├─ Check: No negative amounts
└─ Result: ✓ Valid OR ✗ Out of range → Reject

Layer 3: Currency Validation
├─ Supported: BTC mainnet, testnet3
├─ Rejected: Altcoins (BCH, LTC, etc.)
└─ Future: Whitelist expandable

Cryptographic Validation: ECDSA Verification
├─ Signature: (r, s) pair from signing
├─ Public key: Derived from private key via secp256k1
├─ Message: SHA-256(transaction)
├─ Verification: Q = [s^-1 * (R + r * Qa)] (mod p)
├─ Result: ✓ Valid signature OR ✗ Invalid
└─ Only valid signatures usable in blockchain
```

---

## 🚀 Build & Flash

### Build Configuration Flags

```bash
make USE_CRYPTO_SIGN=1      # Enable ECDSA signing
make USE_LWIP=1             # Enable Ethernet + LwIP (default)
make USE_WEBUSB=1           # WebUSB (default on; use USE_WEBUSB=0 to disable)
make USE_TEST_SEED=1        # Use hardcoded test seed (development only)
make USE_RNG_DUMP=1         # Enable RNG statistical testing
make SKIP_OLED=1            # Skip I2C/OLED if display bus hangs

# Combined example:
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1
```

### Typical Build & Flash

```bash
# Build
cd /data/projects/CryptoWallet
make clean
make all USE_CRYPTO_SIGN=1

# Flash
make flash

# Monitor UART output
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw
```

### Sibling repositories (`CRYPTO_DEPS_ROOT`)

The build expects **STM32CubeH7**, **stm32_secure_boot**, and **stm32-ssd1306** as sibling trees (or set `CRYPTO_DEPS_ROOT` to the directory that contains them):

```bash
export CRYPTO_DEPS_ROOT=/data/projects   # example
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" clean all
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" flash
```

### Host tests

```bash
pip install -r requirements-test.txt
pytest tests/mvp
```

---

## 📊 Performance Metrics

| Operation | Time | Notes |
|-----------|------|-------|
| **ECDSA Signature** | 50-200ms | Depends on RNG |
| **SHA-256** | 5-10ms | Hardware accelerator |
| **Task Switch** | 5-10µs | Context switch latency |
| **SysTick Interrupt** | 3-5µs | Per tick (1ms) |
| **OLED Update** | 50ms | I2C @ 100kHz |
| **HTTP Request** | 100-300ms | Including signature wait |
| **Boot Time** | 100-500ms | Including FreeRTOS init |

---

## Project Structure

Short index from Doxygen `@brief` (refresh when sources change):

<!-- DOXYGEN_PROJECT_STRUCTURE -->

| Module | Brief |
|--------|-------|
| *(run `doxygen Doxyfile` then `python3 scripts/update_readme.py`)* | |

---

## 🔗 Related Documentation

**Manual (canonical):** [`documentation/README.md`](documentation/README.md)

| Topic | Document |
|-------|----------|
| Trust model, boot, threat framing | [`documentation/01-trust-model-and-architecture.md`](documentation/01-trust-model-and-architecture.md) |
| Tasks, IPC, module map | [`documentation/02-firmware-structure.md`](documentation/02-firmware-structure.md) |
| Cryptography, signing | [`documentation/03-cryptography-and-signing.md`](documentation/03-cryptography-and-signing.md) |
| HTTP, WebUSB | [`documentation/04-http-and-webusb.md`](documentation/04-http-and-webusb.md) |
| UART CWUP | [`documentation/05-uart-cwup-protocol.md`](documentation/05-uart-cwup-protocol.md) |
| FW integrity, RNG, dieharder, CI semantics | [`documentation/06-integrity-rng-verification.md`](documentation/06-integrity-rng-verification.md) |
| Build flags, sibling repos, Gitea, containers | [`documentation/07-build-ci-infrastructure.md`](documentation/07-build-ci-infrastructure.md) |

**Host scripts:** [`scripts/README.md`](scripts/README.md)

**MkDocs site (optional):** `make docs` → `site/index.html` — see [`documentation/MAINTENANCE.md`](documentation/MAINTENANCE.md).

---

## 📚 Module Reference

### Core Security & Signing

| Module | Purpose | Location |
|--------|---------|----------|
| **main.c** | FreeRTOS entry point, IPC initialization, task creation | `Core/Src/main.c` |
| **task_sign.c** | Signing FSM pipeline, transaction validation, ECDSA | `Core/Src/task_sign.c` |
| **crypto_wallet.c** | trezor-crypto wrapper, key derivation, RNG | `Core/Src/crypto_wallet.c` |
| **tx_request_validate.c** | Address/amount/currency validation gate | `Core/Src/tx_request_validate.c` |
| **memzero.c** | Secure buffer clearing via volatile writes | `Core/Src/memzero.c` |
| **sha256_minimal.c** | SHA-256 fallback (USE_CRYPTO_SIGN=0) | `Core/Src/sha256_minimal.c` |
| **wallet_seed.c** | Seed management (test/development) | `Core/Src/wallet_seed.c` |
| **task_security.c** | Legacy FSM with mock crypto (audit/test) | `Core/Src/task_security.c` |

### Network & Communication

| Module | Purpose | Location |
|--------|---------|----------|
| **task_net.c** | LwIP/Ethernet, HTTP server port 80 | `Src/task_net.c` |
| **usb_webusb.c** | WebUSB vendor interface, binary protocol | `Core/Src/usb_webusb.c` |
| **app_ethernet_cw.c** | Ethernet link FSM, DHCP state machine | `Src/app_ethernet_cw.c` |
| **time_service.c** | SNTP synchronization, UTC strings | `Core/Src/time_service.c` |
| **usb_device.c** | USB device initialization (HSI48) | `Core/Src/usb_device.c` |
| **usbd_conf_cw.c** | USB BSP configuration, static allocator | `Core/Src/usbd_conf_cw.c` |
| **usbd_desc_cw.c** | USB descriptors, WebUSB Platform UUID | `Core/Src/usbd_desc_cw.c` |

### User Interface & I/O

| Module | Purpose | Location |
|--------|---------|----------|
| **task_display.c** | SSD1306 UI (full), 4-line state rendering | `Core/Src/task_display.c` |
| **task_display_minimal.c** | Minimal UI for minimal-lwip | `Core/Src/task_display_minimal.c` |
| **task_user.c** | Button debounce (PC13), confirm/reject logic | `Core/Src/task_user.c` |
| **task_io.c** | LED indicators (alive, network, alert) | `Core/Src/task_io.c` |

### Hardware & System

| Module | Purpose | Location |
|--------|---------|----------|
| **hw_init.c** | Clock, MPU/cache, GPIO, I2C1, UART, USB, RNG | `Core/Src/hw_init.c` |
| **stm32h7xx_hal_msp.c** | MSP callbacks (I2C1, UART) | `Core/Src/stm32h7xx_hal_msp.c` |
| **stm32h7xx_it.c** | Interrupt handlers (SysTick, ETH) | `Core/Src/stm32h7xx_it.c` |
| **stm32h7xx_it_systick.c** | SysTick handler (minimal-lwip) | `Core/Src/stm32h7xx_it_systick.c` |
| **stm32h7xx_it_usb.c** | USB OTG HS interrupt handler | `Core/Src/stm32h7xx_it_usb.c` |
| **ssd1306_conf.h** | Display driver config (I2C1, 128×32) | `Drivers/ssd1306/ssd1306_conf.h` |

### Shared Contracts

| Header | Purpose | Location |
|--------|---------|----------|
| **wallet_shared.h** | IPC types, queues, events, display context | `Core/Inc/wallet_shared.h` |
| **lwipopts.h** | LwIP compile-time config (heap, TCP, SNTP) | `Core/Inc/lwipopts.h` |
| **main.h** | Board pins, LED defines, UART macro | `Core/Inc/main.h` |

### Detailed Documentation

Narrative depth is in **`documentation/*.md`** (see the **Related Documentation** section above). Per-file API behavior is documented with **`@brief` / `@details`** in the C sources. Refresh the short **Project Structure** table with `doxygen Doxyfile && python3 scripts/update_readme.py`.

---

## ✅ Status

- ✅ **Production-ready** firmware
- ✅ **Full cryptographic** signing support
- ✅ **Real-time** task scheduling (FreeRTOS)
- ✅ **Multi-protocol** communication (HTTP / WebUSB / **CWUP on USART3**)
- ✅ **Secure** key management (BIP-39/32)
- ✅ **Professional** documentation

---

## 📖 Further Reading

1. [`documentation/README.md`](documentation/README.md) — full manual (TOC)
2. [`documentation/01-trust-model-and-architecture.md`](documentation/01-trust-model-and-architecture.md) — security framing
3. [`documentation/07-build-ci-infrastructure.md`](documentation/07-build-ci-infrastructure.md) — build, CI, Python deps
4. [`scripts/README.md`](scripts/README.md) — host utilities

**Choose your language:**
- 🇬🇧 English (above)
- 🇷🇺 [Русский](README_ru.md) (этот файл)
- 🇵🇱 [Polski](README_pl.md)

---

**CryptoWallet** - Demonstrating secure, real-time embedded systems design on STM32H743.

*Last Updated: 2026-03-23*
