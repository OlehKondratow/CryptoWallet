\page stm32h7xx_it "stm32h7xx_it: SysTick + ETH IRQ bridge"

# `stm32h7xx_it.c`

<brief>Файл `stm32h7xx_it` реализует критические обработчики прерываний для базового сценария: `SysTick_Handler` обеспечивает связку HAL tick с tick FreeRTOS, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief>

## Краткий обзор
<brief>Файл `stm32h7xx_it` реализует критические обработчики прерываний для базового сценария: `SysTick_Handler` обеспечивает связку HAL tick с tick FreeRTOS, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief>

## Abstract (Synthèse логики)
В embedded FreeRTOS проекте корректная работа задержек и планировщика требует, чтобы SysTick одновременно обновлял:
- “системный” HAL тик (для `HAL_Delay`, например, необходим для I2C/OLED и других HAL-based подсистем),
- tick планировщика FreeRTOS (для switching и timeouts).

Дополнительно, при сетевой сборке нужен IRQ для Ethernet периферии, который разворачивает обработку кадров/событий LwIP/HAL.

## Logic Flow (ISR behavior)
В ISR-логике нет state machine, но есть последовательности:
1. `SysTick_Handler()`:
   - `HAL_IncTick()` — обновление HAL tick счетчика,
   - `xPortSysTickHandler()` — уведомление FreeRTOS.
2. `ETH_IRQHandler()` (только `USE_LWIP`):
   - `HAL_ETH_IRQHandler(&EthHandle)`.

## Прерывания/регистры
Это файлы обработчиков прерываний:
- прямой доступ к регистрами не делается; всё делегировано HAL.
Главная “регистро-логика” — это вызовы в правильном порядке, чтобы HAL и RTOS тик были консистентны.

## Тайминги и условия ветвления
| Условие | Эффект |
|---|---|
| `USE_LWIP` определён | компилируется `ETH_IRQHandler` |
| SysTick | всегда активен (для FreeRTOS и HAL_DELAY) |

## Dependencies
Прямые:
- FreeRTOS: `xPortSysTickHandler()`
- HAL: `HAL_IncTick()`, `HAL_ETH_IRQHandler`, `EthHandle` (external)
- compile-time flags: `USE_LWIP`

## Связи
- `stm32h7xx_it_systick.md` — альтернативный SysTick handler для minimal-lwip сборки.
- `task_display` использует HAL задержки, поэтому HAL tick обязателен.

