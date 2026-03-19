\page stm32h7xx_it_systick "stm32h7xx_it_systick: SysTick for minimal-lwip"

# `stm32h7xx_it_systick.c`

<brief>File `stm32h7xx_it_systick` addresses the situation for minimal-lwip builds: it provides `SysTick_Handler`, which synchronizes HAL tick and FreeRTOS tick to avoid hangs/incorrect behavior when the base `stm32h7xx_it.c` does not contain `SysTick_Handler`.</brief>

## Overview

<brief>File `stm32h7xx_it_systick` addresses the situation for minimal-lwip builds: it provides `SysTick_Handler`, which synchronizes HAL tick and FreeRTOS tick to avoid hangs/incorrect behavior when the base `stm32h7xx_it.c` does not contain `SysTick_Handler`.</brief>

## Abstract (Logic Synthesis)

SysTick in STM32 is a critical interrupt: it powers both HAL timing and FreeRTOS scheduling. In some minimal LwIP configurations (as noted in file comments), the base `stm32h7xx_it.c` may not connect the correct handler, leading to dangerous consequences (up to infinite loop backtraces / WWDG).

`stm32h7xx_it_systick` is the targeted fix, ensuring SysTick is always serviced with the correct calls.

## Logic Flow (ISR)

In the ISR:
1. `HAL_IncTick()` — Update HAL tick (required for `HAL_Delay`)
2. `xPortSysTickHandler()` — Notify FreeRTOS about tick

## Interrupts/Registers

This is an ISR file:
- Processing happens in the SysTick interrupt handler
- Registers are not directly manipulated; HAL/FreeRTOS hooks are used

## Timings

Calls should be fast, otherwise scheduling will be disrupted:
- ISR is limited to two calls, no loops/parsing

## Dependencies

- HAL: `HAL_IncTick`
- FreeRTOS: `xPortSysTickHandler`

## Relations

- `stm32h7xx_it.md` — Base implementation of handlers in "full" build
