\page app_ethernet_cw "app_ethernet_cw: Ethernet link FSM и DHCP state machine"
\related ethernet_link_status_updated
\related DHCP_Thread
\related ethernet_set_led

# `app_ethernet_cw.c`

<brief>Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса.</brief>

## Краткий обзор

Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса.

## Логика потока (Link + DHCP FSM)

### Link-up callback

`ethernet_link_status_updated(netif)` вызывается, когда кабель подключен/отключен:
1. Если link UP и был DOWN:
   - При `LWIP_DHCP=1`: переходит в `DHCP_START` (DHCP-thread начнёт работать)
   - При `LWIP_DHCP=0`: сразу включает LED2, стартует `time_service_start()`
2. Если link DOWN:
   - При `LWIP_DHCP=1`: переходит в `DHCP_LINK_DOWN`
   - При `LWIP_DHCP=0`: включает LED3 (warning)

### DHCP FSM (когда `LWIP_DHCP=1`)

`DHCP_Thread(netif)` работает как конечный автомат:
1. **DHCP_START:** зануляет IP/mask/gw, переходит в WAIT_ADDRESS, гасит оба LED, вызывает `netifapi_dhcp_start()`
2. **DHCP_WAIT_ADDRESS:** опрашивает `dhcp_supplied_address()`:
   - Если IP получен: логирует адрес, включает LED2, стартует `time_service_start()`, переходит в ASSIGNED
   - Если превышен счётчик попыток (>12): fallback на статический IP из `main.h`, логирует, включает LED2, переходит в TIMEOUT
3. **DHCP_LINK_DOWN:** выключает DHCP, включает LED3
4. Повторяет каждые 500 мс

## Зависимости

Прямые:
- LwIP netif callback механизм
- DHCP клиент функции: `netifapi_dhcp_start()`, `dhcp_supplied_address()`
- LED управление: LED2/LED3 GPIO writes
- Time service: `time_service_start()` на IP assignment

Косвенные:
- `app_ethernet.h` (объявляет интерфейсы)
- `task_net.md` (использует IP connectivity)
- `task_io.md` (LED management)

## Связи модулей

- `app_ethernet.md` (объявляет интерфейсы)
- `task_net.md` (HTTP server зависит от link)
- `task_io.md` (LED policy)
- `time_service.md` (NTP sync после DHCP)
