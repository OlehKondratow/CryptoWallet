\page task_net "task_net: LwIP + HTTP server for transaction signing API"
\related Task_Net_Create
\related http_server_thread
\related parse_tx_json
\related parse_tx_form

# `task_net.c` + `task_net.h`

<brief>Модуль `task_net` — сетевой фасад приложения: запускает LwIP/Ethernet, поднимает HTTP-сервер на порту 80, парсит JSON/form POST запросы (`POST /tx`), валидирует транзакции и отправляет их на подпись через `g_tx_queue`.</brief>

## Краткий обзор
<brief>Модуль `task_net` — сетевой фасад приложения: запускает LwIP/Ethernet, поднимает HTTP-сервер на порту 80, парсит JSON/form POST запросы (`POST /tx`), валидирует транзакции и отправляет их на подпись через `g_tx_queue`.</brief>

## Abstract (Synthèse логики)
`task_net` — это точка входа для всех внешних запросов подписания: хост (ПК, мобиль, веб-интерфейс) подключается по Ethernet, отправляет JSON или форму с адресом/суммой/валютой, а модуль парсит, валидирует, показывает на SSD1306 и ставит задачу в очередь для signing task. Без LwIP (`USE_LWIP=0`) модуль — no-op. Бизнес-роль — быть "врата в микроконтроллер" для сетевого клиента.

## Logic Flow (HTTP server FSM)

1. **Инициализация:**
   - `Task_Net_Create()` из `main.c` создаёт FreeRTOS задачу с приоритетом `NET_PRIORITY`.
   - Задача `net_task()` инициализирует `tcpip_init()` (LwIP ядро).
   - Настраивает netif: DHCP (если `LWIP_DHCP=1`) или static IP (из `main.h`).
   - Устанавливает callback на `ethernet_link_status_updated()`.
   - Стартует отдельный поток `http_server_thread()` с 16K стеком.

2. **HTTP Server FSM:**
   - Создаёт TCP слушатель на порту 80.
   - Цикл: `netconn_accept()` ждёт входящего соединения.
   - Получает всё данные в одном вызове `netconn_recv()` (2-сек timeout).

3. **Request parsing:**
   - Парсит HTTP первую строку: `GET /path` или `POST /path`.
   - Для `GET /ping`: возвращает "pong".
   - Для `GET /`: возвращает HTML форму для интерфейса.
   - Для `GET /tx/signed`: возвращает JSON с последней подписью (`g_last_sig` + флаг `g_last_sig_ready`).
   - Для `POST /tx`:
     - Парсит body как JSON (ищет `"recipient"`, `"amount"`, `"currency"`) или form (`recipient=...&amount=...&currency=...`).
     - Валидирует через `tx_request_validate()`.
     - Если валидно, отправляет в `g_tx_queue` для signing и обновляет display через `g_display_queue`.

4. **Ответы:**
   - HTTP 200 с HTML/JSON/text.
   - HTTP 404 для неизвестных путей.

## Прерывания/регистры
`task_net` не трогает ISR/регистры напрямую. Используется LwIP API (netconn, netbuf). Взаимодействие с HAL только через `ethernet_link_status_updated()` (из `app_ethernet_cw.c`).

## Тайминги и условия ветвления

| Endpoint | Метод | Поведение |
|---|---|---|
| `/ping` | GET | Немедленно вернуть "pong" |
| `/` | GET | Отправить HTML форму |
| `/tx/signed` | GET | Отправить JSON с последней подписью или статусом |
| `/tx` | POST | Парсить body (JSON или form), валидировать, отправить в `g_tx_queue` |
| Другое | GET/POST | HTTP 404 |

Timing:
- HTTP request timeout: 2 сек (netconn_set_recvtimeout).
- Queue send timeout: 100 мс (pdMS_TO_TICKS(100)).
- Task main loop: 2 сек delay для обновления display (IP после DHCP).

## Dependencies
Прямые зависимости:
- **FreeRTOS:** xTaskCreate, xQueueSend, vTaskDelay, xSemaphoreTake/Give.
- **LwIP:** netconn API, tcpip_init, netif.
- **wallet_shared.h:** `wallet_tx_t`, `g_tx_queue`.
- **tx_request_validate.h:** валидация перед очередью.
- **task_display.h:** `Task_Display_Log()`, `g_display_queue`, `g_display_ctx_mutex`.
- **app_ethernet.h:** `ethernet_link_status_updated()` callback.
- Глобальные из других модулей: `g_last_sig[]`, `g_last_sig_ready` (из `task_sign`).

## Связи
- `wallet_shared.md` (структура wallet_tx_t и очередь)
- `tx_request_validate.md` (validation перед enqueueing)
- `task_sign.md` (consumer из g_tx_queue)
- `task_display.md` (display updates)
- `app_ethernet_cw.md` (link callbacks и LED feedback)
