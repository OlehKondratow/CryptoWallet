# Архитектура и стиль кода

## Разделение задач по файлам

Каждая задача FreeRTOS вынесена в **отдельный модуль** (пара `.c` + `.h`):

| Модуль | Файлы | Описание |
|--------|-------|----------|
| **Display** | `Core/Src/task_display.c`, `Core/Inc/task_display.h` | SSD1306 OLED, 4 строки с прокруткой, очередь `g_display_queue` |
| **Security** | `Core/Src/task_security.c`, `Core/Inc/task_security.h` | FSM подписания транзакций, очередь `g_tx_queue`, Event Group |
| **Network** | `Src/task_net.c`, `Core/Inc/task_net.h` | LwIP, HTTP POST /tx, `g_tx_queue` → Security |
| **IO** | `Core/Src/task_io.c`, `Core/Inc/task_io.h` | LEDs, User Button, debounce, `g_user_event_group` |

**Общие** типы и хэндлы — в `wallet_shared.h` (очереди, Event Group, mutex, `display_context_t`).

**Связи между задачами:**

```
Net (HTTP) ──┬── g_tx_queue ───────► Security
             └── g_display_queue ──► Display

IO (Button) ── g_user_event_group ──► Security

Security ── g_display_ctx ──────────► Display

Display ── g_i2c_mutex ────────────► I2C (SSD1306)
```

**Вспомогательные модули:**

- `hw_init.c/h` — clock, MPU, GPIO, I2C1, UART
- `time_service.c/h` — SNTP, epoch, UTC string
- `app_ethernet_cw.c` — link callback, DHCP, LED
- `memzero.c/h` — zeroing sensitive data

**Логирование (UART + OLED line 3):**

- `Task_Display_Log(msg)` — единая точка: UART + строка 3 дисплея (s_log_buf)
- `UART_Log(msg)` — только UART (fatal: Error_Handler, stack overflow, malloc fail)

---

## Оценка по Clean Code

### Соответствует

| Принцип | Реализация |
|---------|-------------|
| **Single Responsibility** | Каждая задача — один модуль, одна ответственность |
| **Разделение по файлам** | 1 задача = 1 пара .c/.h |
| **Понятные имена** | `Task_Display_Create`, `Task_Net_Create`, `g_tx_queue` |
| **Малые функции** | `display_lock`, `display_unlock`, `parse_tx_body`, `fsm_signing_step` |
| **Документация** | Doxygen-формат: `@file`, `@brief`, `@param`, `@return` |
| **Константы** | `DISPLAY_STACK_SIZE`, `DEBOUNCE_MS`, `CONFIRM_TIMEOUT_MS` |
| **Структуры** | `wallet_tx_t`, `display_context_t`, `Transaction_Data_t` |

### Отклонения / ограничения

| Проблема | Где | Причина |
|----------|-----|---------|
| **Глобальные хэндлы** | `main.c`, `wallet_shared.h` | FreeRTOS, общий доступ к очередям/Event Group |
| **Смешение уровней** | `task_net` → `Task_Display_Log` | Прямой вызов логирования; допустимо для embedded |
| **`parse_tx_body`** | `task_net.c` | Простой JSON парсинг без библиотеки; для embedded норма |
| **Длинные функции** | `display_task`, `render_four_scroll_lines` | ~50–80 строк; можно разбить |
| **Magic numbers** | `128`, `32`, `384` в буферах | Часть вынесена в `#define`, часть — нет |

### Итог

Стиль: **практичный embedded-подход** с элементами Clean Code. Модульность и чёткое разделение задач соблюдены, глобальные хэндлы и прямые вызовы между задачами связаны с ограничениями FreeRTOS и embedded.
