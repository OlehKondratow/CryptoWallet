\page stm32h7xx_it "stm32h7xx_it: SysTick + ETH IRQ мост"

# `stm32h7xx_it.c`

<brief>Файл `stm32h7xx_it` реализует критичные обработчики прерываний для основного сценария: `SysTick_Handler` связывает HAL tick с FreeRTOS tick, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief>

## Обзор

<brief>Файл `stm32h7xx_it` реализует критичные обработчики прерываний для основного сценария: `SysTick_Handler` связывает HAL tick с FreeRTOS tick, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief>

## Абстракция (синтез логики)

В embedded FreeRTOS проекте корректная работа задержек и планировщика требует, чтобы SysTick одновременно обновлял:
- "системный" HAL тик (нужен для `HAL_Delay`, требуется для I2C/OLED и других HAL-based подсистем)
- tick планировщика FreeRTOS (для переключения и timeouts)

Дополнительно, при сетевой сборке нужен IRQ для Ethernet периферии, чтобы разворачивать обработку кадров/событий LwIP/HAL.

## Поток логики (поведение ISR)

ISR логика не содержит state machine, но есть последовательность:
1. `SysTick_Handler()`:
   - `HAL_IncTick()` — Обновить счётчик HAL tick
   - `xPortSysTickHandler()` — Уведомить FreeRTOS
2. `ETH_IRQHandler()` (только если `USE_LWIP`):
   - `HAL_ETH_IRQHandler(&EthHandle)`

## Прерывания/регистры

Это файлы обработчиков прерываний:
- Прямой доступ к регистрам не выполняется; всё делегировано HAL
- Главная "регистро-логика" — вызывать функции в правильном порядке, чтобы HAL и RTOS ticks были консистентны

## Времена и условия ветвления

| Условие | Эффект |
|---|---|
| `USE_LWIP` определён | `ETH_IRQHandler` компилируется |
| SysTick | Всегда активен (для FreeRTOS и HAL_DELAY) |

## Зависимости

Прямые:
- FreeRTOS: `xPortSysTickHandler()`
- HAL: `HAL_IncTick()`, `HAL_ETH_IRQHandler`, `EthHandle` (внешний)
- Флаги compile-time: `USE_LWIP`

## Связи

- `stm32h7xx_it_systick.md` — Альтернативный SysTick handler для minimal-lwip сборки
- `task_display` использует HAL задержки, поэтому HAL tick обязателен
