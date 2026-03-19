\page main "main: RTOS bootstrap + task wiring"
\related Error_Handler
\related HW_Init
\related HW_Init_Early_LwIP
\related Task_Display_Create
\related Task_Net_Create
\related Task_Sign_Create
\related Task_IO_Create
\related Task_User_Create

# `main.c` + `main.h`

<brief>The `main` module is the system orchestrator: it bootstraps hardware (HAL, LwIP early init, clocks), establishes IPC contracts (queues, semaphores, event groups, global state), initializes time/crypto services, and spawns the core FreeRTOS task family (display, network, signing, IO, user input). Entry point for the entire embedded wallet application.</brief>

## Overview

`main.c` is the "glue code" that transforms independent hardware blocks and application modules into a functioning system. It is **not** a state machine or a continuous worker—instead, it is a single-shot bootstrap sequence that:

1. Initializes hardware in the correct order (critical for LwIP/DMA/cache consistency)
2. Creates inter-process communication (IPC) primitives that all tasks will depend on
3. Instantiates the task family and starts the FreeRTOS scheduler
4. Never returns (the scheduler runs indefinitely)

## Abstract (Logic Synthesis)

**Problem:** An embedded wallet needs to:
- Accept signing requests from network (HTTP) or USB interfaces
- Allow user to confirm/reject via button
- Display status on a 128×32 OLED
- Perform ECDSA signing if cryptography is enabled
- Manage multiple concurrent tasks safely (no race conditions)

**Solution:** `main.c` provides the **architectural contract**:

1. **Hardware initialization order matters:** LwIP stack needs MPU/cache configured before `HAL_Init()`. This is not obvious and is replicated from `lwip_zero` Cube example.

2. **Global state is intentional:** All tasks share:
   - `g_tx_queue`: Transaction requests flow from network layer to signing layer
   - `g_display_queue`: Transaction data (for display) is queued from network
   - `g_user_event_group`: User button events signal confirmation/rejection
   - `g_i2c_mutex`, `g_ui_mutex`, `g_display_ctx_mutex`: Serialize access to I2C bus (OLED) and shared UI state

3. **Task creation order is deterministic:** Display task is created first (logging priority), then network, then signing, then indicators and user input. This ensures early diagnostic output.

4. **Hooks catch catastrophic failures:**
   - `vApplicationMallocFailedHook()`: No memory available
   - `vApplicationStackOverflowHook()`: Task stack exhausted
   - `Error_Handler()`: Final exit point that logs and halts

## Logic Flow (Bootstrap Sequence)

### Phase 0: Pre-Scheduler Setup (Blocking)

```
┌─────────────────────────────────────────────────┐
│ SCB->CCR: Disable unaligned access trap        │
│ (LwIP/protocol stacks may read fields unaligned)│
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HW_Init_Early_LwIP() [if USE_LWIP]              │
│  • MPU region setup (cache, protection)         │
│  • Cortex-M7 cache enable                       │
│  • LwIP memory pool initialization              │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HAL_Init()                                      │
│  • SystemClock_Config() (STM32H7 PLL setup)     │
│  • Enable default HAL tick via SysTick          │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HW_Init()                                       │
│  • GPIO (LED, button) setup                     │
│  • I2C1 for SSD1306                             │
│  • UART3 for logging                            │
│  • USB device (WebUSB)                          │
│  • RNG (entropy) if USE_CRYPTO_SIGN             │
│  • SSD1306 display initialization               │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ time_service_init()                             │
│  • Set up SNTP for clock sync                   │
│  • Initialize RTC or system tick-based time     │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
   ┌───────────────────────────────────┐
   │ if (USE_CRYPTO_SIGN)              │
   │   crypto_rng_init()               │
   │   (trezor-crypto entropy pool)    │
   └───────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ if (!BOOT_TEST):                                │
│  ├─ Create IPC primitives                       │
│  │  • g_tx_queue (4 × wallet_tx_t)              │
│  │  • g_display_queue (4 × Transaction_Data_t) │
│  │  • g_user_event_group                        │
│  │  • g_i2c_mutex, g_ui_mutex, display_mutex   │
│  │                                              │
│  ├─ Validate creation (NULL check)              │
│  │  Error_Handler() if any fails                │
│  │                                              │
│  └─ osKernelInitialize()                        │
│     (FreeRTOS kernel ready, no tasks yet)       │
│                                                 │
│  else (BOOT_TEST):                              │
│  └─ Loop: toggle LED1, display "boot", delay   │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ Task Creation (in order):                       │
│  1. Task_Display_Create()  → logging priority   │
│  2. Task_Net_Create()      → HTTP, Ethernet     │
│  3. Task_Sign_Create()     → FSM for ECDSA      │
│  4. Task_IO_Create()       → LED management     │
│  5. Task_User_Create()     → button handler     │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ Display startup banner on SSD1306               │
│ (if !SKIP_OLED):                                │
│  "CryptoWallet + LwIP Init..."                  │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ osKernelStart()                                 │
│ (Scheduler takes over, never returns)           │
│ (Error_Handler() if osKernelStart fails)        │
└─────────────────────────────────────────────────┘
```

### Phase 1: Task Execution (Continuous)

Once `osKernelStart()` is called, the scheduler runs tasks preemptively:

- **Task_Display**: Reads events, updates SSD1306 @ ~1 Hz
- **Task_Net**: Listens on Ethernet/HTTP, enqueues transactions
- **Task_Sign**: Waits for transactions → shows on display → awaits button → signs
- **Task_IO**: Blinks LED1 (alive), sets LED2 (network status), LED3 (alert)
- **Task_User**: Debounces button → sends confirm/reject to sign task

All shared data access is protected by mutexes; queues and event groups ensure thread-safe handoff.

## Registers and Peripherals

### System Control Block (SCB) - Cortex-M7

| Register/Field | Operation | Reason |
|---|---|---|
| `SCB->CCR` | Clear `SCB_CCR_UNALIGN_TRP_Msk` | LwIP stack may access unaligned fields in protocol headers (TCP/IP). Without this, M7 would throw a hard fault. |

**Why?** The Cortex-M7 CPU can issue faults on unaligned loads/stores. But LwIP uses C structs that may not be word-aligned in memory. Other implementations (newlib, lwip_zero) also disable this trap.

### Peripheral Initialization Order

| Peripheral | Module | Timing | Reason |
|---|---|---|---|
| MPU | `HW_Init_Early_LwIP()` | Before HAL_Init | Cache/DMA consistency for LwIP buffers |
| Clock | `HAL_Init()` | Early | PLL setup, interrupt ticks |
| GPIO | `HW_Init()` | After HAL | LED, button, AF setup |
| I2C1 | `HW_Init()` | After GPIO | SSD1306 init (depends on GPIO AF) |
| UART3 | `HW_Init()` | After GPIO | Logging backend |
| USB | `HW_Init()` | After GPIO | WebUSB device init |
| RNG | `HW_Init()` + `crypto_rng_init()` | After clock | Entropy seeding |
| SSD1306 | `HW_Init()` → `ssd1306.c` | After I2C | Display ready before tasks |

## Timings and Branch Conditions

### Build Flags

| Flag | Type | Behavior |
|---|---|---|
| `USE_LWIP` | Compile-time | Enables `HW_Init_Early_LwIP()` call. Without it, MPU/cache setup skipped. |
| `BOOT_TEST` | Compile-time | **Diagnostic mode:** skips FreeRTOS, loops with `HAL_Delay(500)` and LED toggle. Useful for testing HAL without task scheduler. |
| `SKIP_OLED` | Compile-time (default 0) | If set to 1, skips SSD1306 init and banner display. |
| `USE_CRYPTO_SIGN` | Compile-time | If set, calls `crypto_rng_init()` to seed trezor-crypto entropy pool. |

### Error Conditions

| Condition | Handler | Result |
|---|---|---|
| Queue/semaphore creation fails (NULL check) | `Error_Handler()` | Logs `[ERR]`, infinite loop; system halts. |
| FreeRTOS heap exhausted during task creation | `vApplicationMallocFailedHook()` | Logs `[MALLOC FAIL]`, infinite loop. |
| Task stack overflow detected | `vApplicationStackOverflowHook()` | Logs `[STACK OVF taskname]`, infinite loop. |
| `osKernelStart()` fails | `Error_Handler()` | Should not happen; indicates FreeRTOS configuration error. |

## Global State Initialized in `main()`

### Queues

```c
g_tx_queue = xQueueCreate(4U, sizeof(wallet_tx_t));
```
- **Size:** 4 items
- **Item:** `wallet_tx_t` (transaction request: recipient, amount, currency)
- **Producer:** `task_net.c` (HTTP POST `/tx` handler)
- **Consumer:** `task_sign.c` (signing FSM)
- **Purpose:** Decouples network thread from signing FSM

```c
g_display_queue = xQueueCreate(4U, sizeof(Transaction_Data_t));
```
- **Size:** 4 items
- **Item:** `Transaction_Data_t` (subset of tx for display)
- **Producer:** `task_net.c` (copies from incoming request)
- **Consumer:** `task_display.c` (renders on SSD1306)
- **Purpose:** Display sees transaction immediately; sign FSM may still be processing

### Event Group

```c
g_user_event_group = xEventGroupCreate();
```
- **Bits used:**
  - `USER_CONFIRM_BIT` (bit 0): User pressed button short (< 2.5s)
  - `USER_REJECT_BIT` (bit 1): User held button long (≥ 2.5s)
- **Producer:** `task_user.c` (debounce + timing logic)
- **Consumer:** `task_sign.c` (waiting on event bits via `xEventGroupWaitBits()`)
- **Purpose:** Non-blocking, interrupt-safe signal from button handler

### Mutexes

```c
g_i2c_mutex = xSemaphoreCreateMutex();    // I2C1 bus
g_ui_mutex = xSemaphoreCreateMutex();      // UI text buffer
g_display_ctx_mutex = xSemaphoreCreateMutex(); // display_context_t
```

| Mutex | Protects | Tasks | Timeout |
|---|---|---|---|
| `g_i2c_mutex` | I2C1 hardware (GPIO AF, DR, status) | task_display, task_io | Infinite (no priority inversion risk) |
| `g_ui_mutex` | Formatted text buffer for display | task_display, task_net, task_sign | Infinite |
| `g_display_ctx_mutex` | `g_display_ctx` struct (4 lines of text, cursor) | task_display | Infinite |

**Design note:** Infinite timeout is acceptable here because:
- Each task holds the mutex briefly (< 1 ms)
- No task performs blocking I2C operations while holding mutex
- No nested mutex acquisition (no deadlock risk)

### Global Flags and Buffers

```c
volatile uint8_t  g_security_alert = 0;     // Set by task_sign if validation fails
uint8_t          g_last_sig[64] = {0};      // Last ECDSA signature (r || s)
volatile uint8_t  g_last_sig_ready = 0;    // Flag: signature ready for USB/HTTP response
```

- **`g_security_alert`:** Atomic flag read by task_io to illuminate LED3
- **`g_last_sig`:** Shared signature buffer; protected by implicit FSM ordering (sign task writes, network task reads after `g_last_sig_ready` = 1)
- **`g_last_sig_ready`:** Volatile flag to indicate signature readiness without needing a semaphore

## Dependencies and Relations

### Hardware Layer (Early Init)

```
main.c
 ├─ HW_Init_Early_LwIP()   → MPU, cache config
 ├─ HW_Init()              → GPIO, I2C, UART, USB, RNG
 └─ hw_init.c, hw_init.h   → Device-specific setup
```

### Time and Entropy

```
main.c
 ├─ time_service_init()     → SNTP synchronization, UTC formatting
 ├─ crypto_rng_init()       → Seed trezor-crypto entropy pool [if USE_CRYPTO_SIGN]
 └─ time_service.c, crypto_wallet.c
```

### Task Creation and Messaging

```
main.c
 ├─ Task_Display_Create()   ──┐
 ├─ Task_Net_Create()       ──┼─ Depend on:
 ├─ Task_Sign_Create()      ──┤  • g_tx_queue
 ├─ Task_IO_Create()        ──┤  • g_display_queue
 ├─ Task_User_Create()      ──┤  • g_user_event_group
 └─ (Task_Security not created)  • g_*_mutex
```

### Global Contracts (wallet_shared.h)

```c
// Declared in main.c, declared extern in wallet_shared.h
QueueHandle_t      g_tx_queue;
QueueHandle_t      g_display_queue;
EventGroupHandle_t g_user_event_group;
SemaphoreHandle_t  g_i2c_mutex;
SemaphoreHandle_t  g_ui_mutex;
SemaphoreHandle_t  g_display_ctx_mutex;
display_context_t  g_display_ctx;
volatile uint8_t   g_security_alert;
uint8_t            g_last_sig[64];
volatile uint8_t   g_last_sig_ready;
```

All tasks include `wallet_shared.h` and access these via extern declarations.

## Related Modules

### Direct Dependents (created by main)

- **`task_display.md`** — Display task receives `g_display_queue` events
- **`task_net.md`** — Network task enqueues `g_tx_queue` and `g_display_queue`
- **`task_sign.md`** — Signing FSM consumer of `g_tx_queue`, sets signature flags
- **`task_io.md`** — Indicator task reads `g_security_alert`, LED state management
- **`task_user.md`** — Button handler signals via `g_user_event_group`

### Infrastructure

- **`hw_init.md`** — Hardware initialization order and rationale
- **`time_service.md`** — SNTP and UTC time management
- **`wallet_shared.md`** — IPC type definitions and global contracts
- **`crypto_wallet.md`** — Entropy initialization (if USE_CRYPTO_SIGN)

### Related (not created here)

- **`task_security.c`** — Alternative signing FSM; linked but not instantiated
- **`app_ethernet_cw.md`** — Ethernet link management (called from task_net)
- **`usb_device.md`**, **`usb_webusb.md`** — USB device interface (initialized in hw_init)

## Critical Design Notes

### 1. Why LwIP Early Init?

LwIP's DMA buffers must reside in a memory region with specific cache policies:
- Write-through for TX buffers (avoid coherency issues)
- Cacheable for RX buffers (performance)

The MPU (Memory Protection Unit) must be configured **before** `HAL_Init()` so that when `HAL_Init()` enables the D-Cache, the cache behavior matches the MPU regions. This is replicated from `lwip_zero` and `lwip-uaid-SSD1306` Cube examples.

### 2. Global Queues vs. Callbacks

Why not use task notifications or callbacks?

- **Queues provide buffering:** If the network task enqueues 2 transactions before the sign task runs, both are in the queue (no loss).
- **Decoupling:** Network task doesn't know or care about sign task timing; just enqueues and continues.
- **Deterministic ordering:** FIFO semantics are clear and predictable.

For one-off signals (button press), event groups are more efficient than queues.

### 3. Why Task_Security is Not Created

`task_security.c` implements an alternative signing FSM with mock cryptography. It is linked but not instantiated (`Task_Security_Create()` is never called) because:
- **Production build** uses `task_sign.c` with real ECDSA
- **Audit/test only** might instantiate `task_security.c` to compare signing behavior
- Keeping both linked allows compile-time flexibility without cluttering main.c

### 4. Error Handling Strategy

Three fatal hooks catch unrecoverable errors:
- `vApplicationMallocFailedHook()` — Heap exhausted
- `vApplicationStackOverflowHook()` — Task stack overflow
- `Error_Handler()` — Catch-all (also called if queue creation fails)

All three log to UART and loop infinitely. This is intentional: in an embedded wallet, graceful recovery is less important than detecting the failure (logging) and halting safely.

### 5. Display Logging During Bootstrap

`Task_Display_Log()` is called throughout `main.c` **before** the display task is created. This works because:
- `Task_Display_Log()` writes directly to the SSD1306 and UART
- It does not depend on the task running; it is just a direct function call
- Early diagnostic output is invaluable for debugging boot hangs
