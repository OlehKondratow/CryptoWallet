\page task_io "task_io: LED policy (heartbeat + security alert)"
\related Task_IO_Create

# `task_io.c` + `task_io.h`

<brief>The `task_io` module manages visual safety and status indicators: it periodically updates LED1 as "alive", manages LED2 as network indicator (depending on LwIP build), and enables LED3 when security alert is present.</brief>

## Overview

The `task_io` module manages visual safety and status indicators: it periodically updates LED1 as "alive", manages LED2 as network indicator (depending on LwIP build), and enables LED3 when security alert is present.

## Logic Flow (Simple Control Loop)

Main loop:
1. Once after startup, turns on LED1 and turns off LED3
2. Subsequently every `POLL_MS=100ms`:
   - LED1 forcibly maintained in "ON" state (heartbeat)
   - LED2 periodically toggled only in build without LwIP (`!USE_LWIP`)
   - LED3 set by condition `g_security_alert != 0`

LED2 conditional logic:

| Build Condition | LED2 Behavior |
|-----------------|---------------|
| `USE_LWIP=1` | LED2 not toggled (in current logic — always false after init) |
| `USE_LWIP=0` | LED2 blinks with period depending on tick counter (`tick_count/25`) |

## Interrupts and Registers

No direct ISR and register work. Uses HAL GPIO writes:
- `HAL_GPIO_WritePin(LED*_GPIO_PORT, LED*_PIN, level)`

## Timings and Branching Conditions

| Parameter | Value |
|-----------|-------|
| loop period | `POLL_MS=100ms` |
| stack/priority | `IO_STACK_SIZE=128`, `IO_PRIORITY = idle+1` |
| LED3 state source | `g_security_alert` |

## Dependencies

Direct dependencies:
- Global flag: `g_security_alert` (from `task_sign` or `task_security`)
- GPIO defines: `LED1_GPIO_PORT/PIN`, `LED2_GPIO_PORT/PIN`, `LED3_GPIO_PORT/PIN` (from `main.h`)
- HAL: `stm32h7xx_hal.h` for `HAL_GPIO_WritePin()`
- FreeRTOS: `vTaskDelay()`, task creation

## Module Relationships

- `task_sign.md` (source of `g_security_alert` on errors)
- `task_security.md` (alternate source of security state)
- `hw_init.md` (GPIO init for LEDs)
