\page stm32h7xx_it "stm32h7xx_it: SysTick + ETH IRQ bridge"

# `stm32h7xx_it.c`

<brief>File `stm32h7xx_it` implements critical interrupt handlers for the main scenario: `SysTick_Handler` bridges HAL tick to FreeRTOS tick, and when LwIP is enabled, processes Ethernet IRQ via `HAL_ETH_IRQHandler`.</brief>

## Overview

<brief>File `stm32h7xx_it` implements critical interrupt handlers for the main scenario: `SysTick_Handler` bridges HAL tick to FreeRTOS tick, and when LwIP is enabled, processes Ethernet IRQ via `HAL_ETH_IRQHandler`.</brief>

## Abstract (Logic Synthesis)

In an embedded FreeRTOS project, correct timing and scheduler operation requires SysTick to simultaneously update:
- "System" HAL tick (needed for `HAL_Delay`, required for I2C/OLED and other HAL-based subsystems)
- FreeRTOS scheduler tick (for switching and timeouts)

Additionally, in network builds, an Ethernet IRQ is needed to dispatch frame/event handling to LwIP/HAL.

## Logic Flow (ISR Behavior)

ISR logic has no state machine, but a sequence:
1. `SysTick_Handler()`:
   - `HAL_IncTick()` — Update HAL tick counter
   - `xPortSysTickHandler()` — Notify FreeRTOS
2. `ETH_IRQHandler()` (only if `USE_LWIP`):
   - `HAL_ETH_IRQHandler(&EthHandle)`

## Interrupts/Registers

These are interrupt handler files:
- Direct register access is not done; all delegated to HAL
- Main "register logic" is calling functions in the correct order so HAL and RTOS ticks remain consistent

## Timings and Branch Conditions

| Condition | Effect |
|---|---|
| `USE_LWIP` defined | `ETH_IRQHandler` compiled |
| SysTick | Always active (for FreeRTOS and HAL_DELAY) |

## Dependencies

Direct:
- FreeRTOS: `xPortSysTickHandler()`
- HAL: `HAL_IncTick()`, `HAL_ETH_IRQHandler`, `EthHandle` (external)
- Compile-time flags: `USE_LWIP`

## Relations

- `stm32h7xx_it_systick.md` — Alternative SysTick handler for minimal-lwip build
- `task_display` uses HAL delays, so HAL tick is mandatory
