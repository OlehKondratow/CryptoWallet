\page stm32h7xx_it_systick "stm32h7xx_it_systick: SysTick для minimal-lwip"

# `stm32h7xx_it_systick.c`

<brief>Файл `stm32h7xx_it_systick` разрешает ситуацию для minimal-lwip сборок: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и FreeRTOS tick, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief>

## Обзор

<brief>Файл `stm32h7xx_it_systick` разрешает ситуацию для minimal-lwip сборок: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и FreeRTOS tick, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief>

## Абстракция (синтез логики)

SysTick в STM32 — критичное прерывание: оно питает одновременно HAL тайминг и FreeRTOS планирование. В некоторых конфигурациях минимального LwIP (как указано в комментариях файла) базовый `stm32h7xx_it.c` может не подключить корректный обработчик, что приводит к опасным последствиям (вплоть до infinite loop backtrace / WWDG).

`stm32h7xx_it_systick` — целевой fix, гарантирующий, что SysTick всегда будет обслужен правильными вызовами.

## Поток логики (ISR)

В ISR:
1. `HAL_IncTick()` — Обновить HAL tick (требуется для `HAL_Delay`)
2. `xPortSysTickHandler()` — Уведомить FreeRTOS о тике

## Прерывания/регистры

Это ISR-файл:
- Обработка происходит в обработчике прерывания SysTick
- Регистры напрямую не трогаются; используются HAL/FreeRTOS hooks

## Времена

Вызовы должны быть быстрыми, иначе планирование нарушится:
- ISR ограничен двумя вызовами, без циклов/парсинга

## Зависимости

- HAL: `HAL_IncTick`
- FreeRTOS: `xPortSysTickHandler`

## Связи

- `stm32h7xx_it.md` — Базовая реализация обработчиков в "полной" сборке
