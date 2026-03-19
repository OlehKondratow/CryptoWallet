\page stm32h7xx_it_systick "stm32h7xx_it_systick: SysTick for minimal-lwip"

# `stm32h7xx_it_systick.c`

<brief>Файл `stm32h7xx_it_systick` корректирует ситуацию для сборки minimal-lwip: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и tick FreeRTOS, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief>

## Краткий обзор
<brief>Файл `stm32h7xx_it_systick` корректирует ситуацию для сборки minimal-lwip: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и tick FreeRTOS, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief>

## Abstract (Synthèse логики)
SysTick в STM32 — критичный interrupt: он питает одновременно HAL тайминг и FreeRTOS scheduling. В некоторых конфигурациях минимального LwIP (как указано в комментарии к файлу) базовый `stm32h7xx_it.c` может не подключить корректный обработчик, и это приводит к опасным последствиям (вплоть до infinite loop backtrace / WWDG).

`stm32h7xx_it_systick` — целевой fix, который гарантирует, что SysTick всегда будет обслужен правильными вызовами.

## Logic Flow (ISR)
В ISR:
1. `HAL_IncTick()` — обновить HAL tick (нужно для `HAL_Delay`).
2. `xPortSysTickHandler()` — уведомить FreeRTOS о тике.

## Прерывания/регистры
Это ISR-файл:
- обработка идёт в обработчике прерывания SysTick,
- регистры напрямую не трогаются; используются HAL/FreeRTOS hooks.

## Тайминги
Вызовы должны быть быстрыми, иначе нарушится scheduling:
- ISR ограничен двумя вызовами, без циклов/парсинга.

## Dependencies
- HAL: `HAL_IncTick`
- FreeRTOS: `xPortSysTickHandler`

## Связи
- `stm32h7xx_it.md` — базовая реализация обработчиков в “полной” сборке.

