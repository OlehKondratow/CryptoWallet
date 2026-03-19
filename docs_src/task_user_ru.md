\page task_user "task_user: политика кнопки USER (подтверждение/отклонение)"
\related Task_User_Create

# `task_user.c` + `task_user.h`

<brief>Модуль `task_user` реализует физическую UX-логику для кнопки USER (PC13): делает debounce, различает короткое нажатие (Confirm) и длинное удержание (~2.5s) как Reject и сигналит это в `task_sign` через `g_user_event_group`.</brief>

## Краткий обзор

Модуль `task_user` реализует физическую UX-логику для кнопки USER (PC13): делает debounce, различает короткое нажатие (Confirm) и длинное удержание (~2.5s) как Reject и сигналит это в `task_sign` через `g_user_event_group`.

## Логика потока (button state machine)

Подход: периодический poll с фильтрацией стабильности.

Триггеры и пороги:

| Параметр | Значение |
|----------|-------:|
| `POLL_MS` | 20ms |
| Debounce | 50ms |
| Long press | 2500ms |

Состояния (неявно, через переменные):
1. Idle (кнопка не нажата)
2. Pressed-Stabilizing (кнопка нажата, идёт накопление стабильных тиков)
3. Confirm-Fired (короткое нажатие подтверждено при отпускании)
4. Reject-Fired (длинное удержание отклоняет сразу во время удержания)

Правила:
1. Пока кнопка остаётся нажатой — накапливается `btn_stable_ticks`
2. Если удержание достигает `LONG_PRESS_MS` и действие ещё не "fired" — ставится `EVENT_USER_REJECTED`
3. При отпускании:
   - если длительность в диапазоне `[DEBOUNCE_MS, LONG_PRESS_MS)` и действие ещё не "fired" — ставится `EVENT_USER_CONFIRMED`
   - затем сбрасываются флаги длительности

## Прерывания и регистры

Прямых ISR нет: модуль работает как FreeRTOS task и читает состояние GPIO через HAL.
Особенность таймингов — точность на уровне `POLL_MS` (20ms): событие может "сдвинуться" не более чем на один тик.

## Тайминги и условия ветвления

| Ветвление | Условие | Результат |
|----------|---|---|
| Reject | `last_press_duration >= 2500 && !action_fired` | set `EVENT_USER_REJECTED` |
| Confirm | `DEBOUNCE_MS <= last_press_duration < 2500 && !action_fired` при отпускании | set `EVENT_USER_CONFIRMED` |
| Debounce | пока длительность не достигла 50ms | события не ставятся |

## Зависимости

Прямые:
- Вход: `USER_KEY_GPIO_PORT`, `USER_KEY_PIN`, `USER_KEY_PRESSED` (из `main.h`)
- События: `g_user_event_group` + биты `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (из `wallet_shared.h` / архитектуры задач)
- Логирование: `Task_Display_Log`

Косвенные:
- `task_sign` ожидает эти event bits и переводит систему в подтверждающий/отклоняющий сценарий

## Связи модулей

- `task_sign.md` (consumer `g_user_event_group`)
- `task_io.md` (индикация security alert после решения)
- `task_display.md` (подписи "Confirm/Reject" через лог-слой)
