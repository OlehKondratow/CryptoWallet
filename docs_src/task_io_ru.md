\page task_io "task_io: политика LED (heartbeat + security alert)"
\related Task_IO_Create

# `task_io.c` + `task_io.h`

<brief>Модуль `task_io` отвечает за визуальные индикаторы безопасности и статуса системы: он периодически обновляет LED1 как "alive", управляет LED2 как сетевым индикатором (в зависимости от сборки LwIP) и включает LED3 при наличии security alert.</brief>

## Краткий обзор

Модуль `task_io` отвечает за визуальные индикаторы безопасности и статуса системы: он периодически обновляет LED1 как "alive", управляет LED2 как сетевым индикатором (в зависимости от сборки LwIP) и включает LED3 при наличии security alert.

## Логика потока (простой control loop)

Основной цикл:
1. Один раз после старта включает LED1 и выключает LED3
2. В дальнейшем каждую `POLL_MS=100ms`:
   - LED1 принудительно поддерживается в "ON" состоянии (heartbeat)
   - LED2 периодически переключается только в сборке без LwIP (`!USE_LWIP`)
   - LED3 выставляется по условию `g_security_alert != 0`

Условная логика LED2:

| Условие сборки | Поведение LED2 |
|---|---|
| `USE_LWIP=1` | LED2 не переключается (в текущей логике — всегда false после init) |
| `USE_LWIP=0` | LED2 мигает с периодом, зависящим от счётчика тиков (`tick_count/25`) |

## Прерывания и регистры

Прямых ISR и работы с регистрами нет. Используются HAL GPIO записи:
- `HAL_GPIO_WritePin(LED*_GPIO_PORT, LED*_PIN, level)`

## Тайминги и условия ветвления

| Параметр | Значение |
|----------|-------:|
| период цикла | `POLL_MS=100ms` |
| stack/priority | `IO_STACK_SIZE=128`, `IO_PRIORITY = idle+1` |
| источник состояния LED3 | `g_security_alert` |

## Зависимости

Прямые зависимости:
- Глобальный флаг: `g_security_alert` (из `task_sign` или `task_security`)
- GPIO defines: `LED1_GPIO_PORT/PIN`, `LED2_GPIO_PORT/PIN`, `LED3_GPIO_PORT/PIN` (из `main.h`)
- HAL: `stm32h7xx_hal.h` для `HAL_GPIO_WritePin()`
- FreeRTOS: `vTaskDelay()`, создание задачи

## Связи модулей

- `task_sign.md` (источник `g_security_alert` при ошибках)
- `task_security.md` (альтернативный источник security state)
- `hw_init.md` (GPIO init для LED)
