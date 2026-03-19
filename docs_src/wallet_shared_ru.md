\page wallet_shared "wallet_shared: контракты IPC/данных для net/display/sign/user"
\related g_tx_queue
\related g_display_queue
\related g_user_event_group
\related g_i2c_mutex
\related g_ui_mutex
\related g_display_ctx_mutex

# `wallet_shared.h` (контракт данных + IPC)

<brief>Заголовок `wallet_shared` задаёт единый контракт между модулями: структуры данных для "запрос → подтверждение → подпись" и UI-состояния, а также global IPC-ручки (очереди, event group, mutex'ы), которыми обмениваются `task_net`, `task_sign`, `task_user` и `task_display`.</brief>

## Краткий обзор

Заголовок `wallet_shared` задаёт единый контракт между модулями: структуры данных для "запрос → подтверждение → подпись" и UI-состояния, а также global IPC-ручки (очереди, event group, mutex'ы), которыми обмениваются `task_net`, `task_sign`, `task_user` и `task_display`.

## Логика потока

В архитектуре embedded-системы "бизнес-логика" приложения разнесена по задачам, а `wallet_shared.h` — это слой договорённостей: как именно модуль сети превращается в объект, понятный модулю подписания, как UX-кнопка превращается в дискретное событие, и как статус безопасного/подписанного кошелька превращается в то, что рисует SSD1306. Без этого контракта любое разнесение задач по потокам превращается в хаотичную передачу данных.

## Формальная модель

Явной state machine в заголовке нет, но есть "формальная" модель:
1. Сеть/хост формирует `wallet_tx_t` (recipient/amount/currency)
2. `task_sign` потребляет `wallet_tx_t` из `g_tx_queue` и переводит UI через `g_display_ctx`
3. `task_user` выставляет события в `g_user_event_group`, которые `task_sign` ждёт
4. UI получает обновления либо через очередь `g_display_queue`, либо через `g_display_ctx`

## Прерывания и регистры

Заголовок только объявляет типы/глобальные дескрипторы. Никаких регистров/ISR не содержит.

## Зависимости

Ключевые зависимости по данным:
- Типы и размеры строк, согласованные с валидацией/рендером:
  - `TX_RECIPIENT_LEN`, `TX_AMOUNT_LEN`, `TX_CURRENCY_LEN`
- Пользовательские события:
  - `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (передаются в `task_sign`)
- Состояния/контекст UI:
  - `display_state_t` (набор экранов) и `display_context_t` (данные для отрисовки)
- Состояния signing FSM:
  - `signing_state_t` как общий enum (и для активного task_sign, и для legacy task_security)

Глобальные handles (создаются в `main.c`, используются в задачах):

| Handle | Тип | Обычно пишет | Читает/ждёт |
|--------|-----|---|---|
| `g_tx_queue` | `QueueHandle_t` | `task_net` | `task_sign` |
| `g_display_queue` | `QueueHandle_t` | `task_net` | `task_display` |
| `g_user_event_group` | `EventGroupHandle_t` | `task_user` | `task_sign` |
| `g_i2c_mutex` | `SemaphoreHandle_t` | display | display-only критическая секция |
| `g_ui_mutex` | `SemaphoreHandle_t` | task_display | merge контекста |
| `g_display_ctx_mutex` | `SemaphoreHandle_t` | net/sign | чтение в display |

Глобальные переменные состояния/результата:
- `g_security_alert` — индикатор, который мапится на LED3
- `g_last_sig[64]` и `g_last_sig_ready` — последняя подпись, опрашиваемая сетевым/USB слоем

## Связи модулей

- Consumer подписи: `task_sign.md`
- Consumer/UX events: `task_user.md`
- Валидация host payload: `tx_request_validate.md`
- Витрина/UI: `task_display.md`
- Политика отображения alert: `task_io.md`
- Сетевая сборка payload/очередей: `task_net.md`, `app_ethernet_cw.md`
