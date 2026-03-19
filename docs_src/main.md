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

<brief>The `main` module describes the "glue" for the entire STM32H7 application: it establishes the order of early HAL/LwIP initialization, creates IPC objects (queues, semaphores, event groups), launches critical FreeRTOS tasks, and starts the scheduler.</brief>

## Overview
<brief>The `main` module describes the "glue" for the entire STM32H7 application: it establishes the order of early HAL/LwIP initialization, creates IPC objects (queues, semaphores, event groups), launches critical FreeRTOS tasks, and starts the scheduler.</brief>

## Abstract (Logic Synthesis)
`main` is the entry point that transforms a collection of hardware blocks (clocks, peripherals) and application tasks (display/net/sign/io/user) into a working system. All "business logic" is distributed across tasks, but `main` establishes the data contract: which global structures serve as event channels, which mutexes protect shared resources (I2C/UI), and which startup flags are picked up by downstream layers (e.g., time/cache for LwIP).

### Critical Invariants
1. Before `HAL_Init()`, an early branch aligned with LwIP (MPU/cache) runs.
2. Before task creation, IPC objects and mutexes are created so tasks don't work with NULL descriptors.
3. Initialization order matches the "reference" LwIP/Cube style (lwip_zero / lwip-uaid-SSD1306).

## Logic Flow (bootstrap sequence)
There's no linear system-level state machine, but a sequence of phases:

1. Hardware preparation (bypass unaligned-access exception for M7).
2. Early LwIP configuration package (only if `USE_LWIP`).
3. Standard HAL initialization, then `HW_Init()` (GPIO/I2C/UART/USB/RNG).
4. Time service initialization.
5. Conditional crypto-RNG enablement.
6. In "production scenario" (not `BOOT_TEST`): queue/semaphore/event group creation and core startup.
7. Task creation in fixed order (display → net → sign → io → user).
8. Scheduler startup.

## Interrupts/Registers
Direct peripheral register manipulation in `main.c` is minimal; the main "low-level" step is tweaking system behavior:

| Action | Reason |
|---|---|
| `SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk` | Permit unaligned access where LwIP stack/structures may read protocol fields unaligned. |

## Timings and Branch Conditions

| Condition | Result |
|---|---|
| `BOOT_TEST` | Instead of FreeRTOS, run diagnostic loop with `HAL_Delay(500)`. |
| `SKIP_OLED` | Banner on SSD1306 is not drawn (SSD1306 already initialized in `HW_Init()`). |
| `USE_CRYPTO_SIGN` | Raise `crypto_rng_init()` (if crypto enabled). |

## Dependencies
Direct data/call relationships:

- **Hardware:** `HW_Init_Early_LwIP()`, `HW_Init()`, `HAL_Init()`.
- **Time:** `time_service_init()`.
- **IPC & task contract:** `g_tx_queue`, `g_display_queue`, `g_user_event_group`, `g_i2c_mutex`, `g_ui_mutex`, `g_display_ctx_mutex`, `g_display_ctx`.
- **Tasks:** `Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`.
- **UI/logging:** `Task_Display_Log()` and indirectly `UART_Log()` (via display layer).

Global structures/flags created/initialized in `main`:

- **Queues:** `g_tx_queue` (net → sign), `g_display_queue` (tx snapshot → display).
- **Event group:** `g_user_event_group` (user → sign confirm/reject).
- **Mutexes:** protect I2C/UI/display context.
- **Flags/status:** `g_security_alert` and `g_last_sig_ready` stay at defaults (managed by tasks later).

## Relations
- Hardware layer: `hw_init.md`
- Display/logging: `task_display.md`, `task_display_minimal.md`
- Network: `task_net.md`, `app_ethernet_cw.md`
- Confirmation/security: `task_user.md`, `task_sign.md`, `task_security.md`
- Time: `time_service.md`
