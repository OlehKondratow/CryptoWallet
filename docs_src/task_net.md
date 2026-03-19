\page task_net "task_net: LwIP + HTTP edge (POST /tx, GET status)"
\related Task_Net_Create
\related tx_request_validate
\related g_tx_queue
\related g_display_queue
\related Task_Display_Log
\related http_server_thread

# `task_net.c` + `task_net.h`

<brief>Модуль `task_net` выступает сетевым “входом” для кошелька: он поднимает LwIP, запускает HTTP-сервер на порту 80, принимает запросы на подпись (`POST /tx`), валидирует входные строки и передаёт подтверждение-подпись в `task_sign` через `g_tx_queue`, одновременно подготавливая отображение pending-статуса через `g_display_queue`.</brief>

## Краткий обзор
<brief>Модуль `task_net` выступает сетевым “входом” для кошелька: он поднимает LwIP, запускает HTTP-сервер на порту 80, принимает запросы на подпись (`POST /tx`), валидирует входные строки и передаёт подтверждение-подпись в `task_sign` через `g_tx_queue`, одновременно подготавливая отображение pending-статуса через `g_display_queue`.</brief>

## Abstract (Synthèse логики)
Бизнес-задача `task_net` — превратить внешние HTTP-вызовы в структурированный “контракт” для подписания на микроконтроллере. Это пограничный слой (edge): на нём сходятся TCP/IP стек, парсинг минимального формата входных данных (JSON/form без тяжёлых библиотек) и строгий валидатор. Важно, что `task_net` не подписывает и не реализует подтверждение — он лишь формирует корректные данные, ставит их в очередь на подпись и управляет тем, что пользователь увидит на дисплее в момент ожидания подтверждения.

## Logic Flow (LwIP init + HTTP request routing)
### Фаза A: старт сети (`net_task`)
Логическая последовательность:
1. Старт задачи “Net” и печать диагностик в UI-лог.
2. Инициализация TCP/IP подсистемы:
   - `tcpip_init(NULL, NULL)` — запуск обработчика в контексте LwIP.
3. Формирование адресов:
   - если `LWIP_DHCP` — IP/netmask/gw обнуляются;
   - иначе — подставляется статический fallback (`IP_ADDR0..3`, `NETMASK_*`, `GW_*` из `main.h`/макросов).
4. Добавление netif и настройка default:
   - `netif_add(..., &ethernetif_init, &tcpip_input)` и `netif_set_default`.
5. Назначение link-статуса:
   - вызов `ethernet_link_status_updated(&g_netif)` для синхронизации LED/UI.
6. Подключение callbacks / потоков:
   - если не используется link callback — вручную поднимается netif (`netif_set_up` и `netif_set_link_up`);
   - если используется link callback — регистрируется `netif_set_link_callback` и стартует поток `EthLink`;
   - если `LWIP_DHCP` — стартует поток `DHCP` через `sys_thread_new`.
7. Заполнение поля адресов в `g_display_ctx`:
   - под mutex читается `netif_ip4_addr` и формируется строка IP + MAC.
8. Запуск HTTP thread:
   - стартует `sys_thread_new("HTTP", http_server_thread, ..., 16384, 2)`.
9. Основной бесконечный цикл:
   - каждые ~2 секунды под mutex обновляется IP string (если DHCP назначил адрес),
   - опционально печатается “Net: alive” (когда включён `LWIP_ALIVE_LOG`).

Конфигурационные параметры:
| Параметр | Значение | Зачем |
|---|---:|---|
| stack Net task | `NET_STACK_SIZE=1024` | достаточно для LwIP init wrapper |
| HTTP thread stack | `16384` | HTTP-обработчик использует буферы/парсинг |
| recv timeout | `2000` мс | ограничение ожидания запроса в HTTP thread |
| HTTP port | `80` | простой интерфейс без шифрования |

### Фаза B: HTTP сервер и маршрутизация эндпоинтов (`http_server_thread`)
Обработка запроса (внутри одного потока):
1. Создать TCP listener (`netconn_new(NETCONN_TCP)`), привязаться к `HTTP_PORT=80`, `netconn_listen()`.
2. В цикле:
   - принять соединение (`netconn_accept`),
   - поставить timeout на прием (`netconn_set_recvtimeout(conn, 2000)`),
   - принять один буфер запроса (`netconn_recv`) и скопировать его в локальный `req` (размер ограничен `REQ_BUF_SIZE=1024`).
3. Из `req` выделяется путь:
   - поиск “GET ” или “POST ” по префиксу,
   - извлечение фрагмента между первым и вторым пробелом.
4. Ветвление по методу/пути:
   - `GET /ping` -> ответ `pong` (test connectivity),
   - `GET /` -> HTML-страница формы отправки на `/tx`,
   - `GET /tx/signed` -> JSON:
     - если `g_last_sig_ready=1` — отдаёт подпись в hex,
     - иначе — возвращает `"pending"`,
   - `POST /tx` -> принимает body:
     - находит начало тела по `"\r\n\r\n"`,
     - определяет payload-тип по строке `application/json`:
       - JSON: достаёт `recipient/amount/currency` через поиск подстрок и кавычек,
       - иначе: form-urlencoded парсит `recipient=&amount=&currency=` по `&`.
     - далее вызывает `tx_request_validate`:
       - при ошибке валидации логирует `"[HTTP] TX invalid"`,
       - при успехе отправляет `wallet_tx_t` в `g_tx_queue` с timeout `100ms`.
     - если enqueue успешен — формирует `Transaction_Data_t` для дисплея и ставит в `g_display_queue` (также `100ms`), выставляя `is_pending=1`.
     - возвращает HTML “Transaction submitted… Confirm on device”.
   - любые прочие пути -> 404.
5. После ответа соединение закрывается (`netconn_close`, `netconn_delete`).

## Прерывания/регистры
`task_net` работает поверх LwIP и FreeRTOS:
- прямых манипуляций аппаратными регистрами нет;
- но критически важна корректность потоков и контекстов:
  - `tcpip_init()` должен запуститься до построения netif/HTTP обработчиков,
  - callback link status и DHCP thread работают как отдельные LwIP-потоки.

## Тайминги и условия ветвления (критичные)
| Условие | Тайминг/ограничение | Что происходит |
|---|---|---|
| `netconn_recv` | `2000ms` recv timeout | при отсутствии данных HTTP loop не продолжает парсинг |
| лимит входного `req` | `REQ_BUF_SIZE=1024` | запросы больше не принимаются целиком |
| `xQueueSend(g_tx_queue)` | `pdMS_TO_TICKS(100)` | если очередь занята — pending не запускается |
| `xQueueSend(g_display_queue)` | `pdMS_TO_TICKS(100)` | UI не обновляется при перегрузе display queue |
| DHCP vs static | compile-time `LWIP_DHCP` | влияет на адреса и момент старта `time_service_start()` через `app_ethernet_cw.c` |

## Dependencies
Прямые зависимости:
- LwIP: `lwip/netif.h`, `lwip/tcpip.h`, `ethernetif.h`, `app_ethernet.h` (link callback + DHCP FSM).
- Контракт данных: `wallet_shared.h` (тип `wallet_tx_t`).
- Слой подтверждения/подписи: `task_sign` через `g_tx_queue`.
- UI-слой: `task_display.h` / `Transaction_Data_t`, `g_display_ctx_mutex`, `Task_Display_Log()`.
- Валидация: `tx_request_validate.h`.

Изменяемые глобальные структуры:
- `g_display_ctx.ip_addr`, `g_display_ctx.mac_addr` (под `g_display_ctx_mutex`).
- Очереди:
  - `g_tx_queue` (enqueue `wallet_tx_t`),
  - `g_display_queue` (enqueue `Transaction_Data_t` с `is_pending=1`).
- Использует `g_last_sig_ready` и `g_last_sig` для ответа `GET /tx/signed` (без mutex в текущей логике).

## Связи
- `app_ethernet_cw.md`: DHCP, link callback, LED2/LED3, старт time_service.
- `tx_request_validate.md`: “ворота” для host payload.
- `task_sign.md`: consumer `g_tx_queue` и ожидание confirm.
- `task_display.md` / `task_display_minimal.md`: UI обновления и лог-строка.

