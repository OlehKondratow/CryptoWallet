\page app_ethernet_cw "app_ethernet_cw: Ethernet link FSM and DHCP state machine"
\related ethernet_link_status_updated
\related DHCP_Thread
\related ethernet_set_led

# `app_ethernet_cw.c`

<brief>Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса.</brief>

## Краткий обзор
<brief>Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса.</brief>

## Abstract (Synthèse логики)
Между LwIP/netif (low-level TCP/IP stack) и приложением нужен "мост" для реакции на события link и IP-адрес-назначение. `app_ethernet_cw` предоставляет этот мост: хук `ethernet_link_status_updated()` вызывается при изменении состояния кабеля Ethernet, DHCP FSM управляет стартом/остановкой DHCP клиента и fallback на статический IP. Глобальные флаги LED (LED2 на сеть, LED3 на no-link) дают визуальный feedback для пользователя. Бизнес-роль — гарантировать, что при включении/отключении кабеля приложение корректно обнулит IP и переконфигурируется.

## Logic Flow (Link + DHCP FSM)

### Link-up callback
`ethernet_link_status_updated(netif)` вызывается, когда кабель подключен/отключен:
1. Если link UP и был DOWN:
   - При `LWIP_DHCP=1`: переходит в `DHCP_START` (DHCP-thread начнёт работать).
   - При `LWIP_DHCP=0`: сразу включает LED2, стартует `time_service_start()`.
2. Если link DOWN:
   - При `LWIP_DHCP=1`: переходит в `DHCP_LINK_DOWN`.
   - При `LWIP_DHCP=0`: включает LED3 (warning).

### DHCP FSM (когда `LWIP_DHCP=1`)
`DHCP_Thread(netif)` работает как конечный автомат:
1. **DHCP_START:** зануляет IP/mask/gw, переходит в WAIT_ADDRESS, гасит оба LED, вызывает `netifapi_dhcp_start()`.
2. **DHCP_WAIT_ADDRESS:** опрашивает `dhcp_supplied_address()`:
   - Если IP получен: логирует адрес, включает LED2, стартует `time_service_start()`, переходит в ASSIGNED.
   - Если превышен счётчик попыток (>12): fallback на статический IP из `main.h`, логирует, включает LED2, переходит в TIMEOUT.
3. **DHCP_LINK_DOWN:** выключает DHCP, включает LED3.
4. Повторяет каждые 500 мс.

### LED management
- **LED2 (network active):** 1 = есть IP (DHCP или static).
- **LED3 (link down warning):** 1 = кабель отключен.

## Прерывания/регистры
Модуль трогает GPIO через HAL:
- `HAL_GPIO_WritePin()` для LED2/LED3 (определены в `main.h`).
- LwIP callbacks в контексте (не ISR, но может быть из разных тредов).

## Тайминги и условия ветвления

| Событие | Условие | Действие |
|---|---|---|
| Link goes UP | netif.flags & NETIF_FLAG_LINK_UP | Если DHCP: DHCP_START; если static: LED2 + time_service_start |
| Link goes DOWN | !(netif.flags & NETIF_FLAG_LINK_UP) | Если DHCP: DHCP_LINK_DOWN + LED3 |
| DHCP address received | dhcp_supplied_address(netif) | DHCP_ADDRESS_ASSIGNED + LED2 + time_service_start |
| DHCP timeout | dhcp->tries > MAX_DHCP_TRIES (12) | Static fallback IP + LED2 + time_service_start |
| DHCP main loop | каждые 500 мс | State machine iteration |

## Dependencies
Прямые зависимости:
- **LwIP:** `netif.h`, `netifapi.h`, `dhcp.h`, `ip4_addr.h`.
- **HAL GPIO:** из `hw_init.c` (LED2/LED3 пины через GPIO).
- **task_display.h:** `Task_Display_Log()` для логирования.
- **time_service.h:** `time_service_start()` при получении IP.
- Глобальные флаги: `DHCP_state` (когда `LWIP_DHCP=1`).

## Связи
- `hw_init.md` (LED GPIO инициализация)
- `task_net.md` (использует callbacks из этого модуля)
- `time_service.md` (стартует при IP assignment)
