\page app_ethernet "app_ethernet: интерфейсы Ethernet glue (link callback + DHCP FSM)"

# `Core/Inc/app_ethernet.h`

<brief>Заголовок `app_ethernet` задаёт интерфейсы Ethernet "glue" слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`.</brief>

## Краткий обзор

Заголовок `app_ethernet` задаёт интерфейсы Ethernet "glue" слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`.

## Логика потока (контракт + model состояния)

Run-time state machine реализована в `app_ethernet_cw.c`, но состояние формализовано здесь константами:

| Состояние | Значение | Смысл |
|-----------|-------:|---|
| `DHCP_OFF` | 0 | DHCP не активен |
| `DHCP_START` | 1 | переход к старту DHCP после link-up |
| `DHCP_WAIT_ADDRESS` | 2 | ожидание lease/адреса |
| `DHCP_ADDRESS_ASSIGNED` | 3 | адрес получен |
| `DHCP_TIMEOUT` | 4 | DHCP не удался, выбирается static fallback |
| `DHCP_LINK_DOWN` | 5 | link упал, DHCP надо остановить/сбросить |

## Прерывания и регистры

Заголовок не содержит ISR и регистров — только объявления.

## Зависимости

Объявление функций и константы состояния:
- `ethernet_link_status_updated()` — вызывается LwIP при изменении link
- `DHCP_Thread()` — main loop потока для FSM DHCP
- Значения enum состояния, используемые `app_ethernet_cw.c`

## Связи модулей

- `app_ethernet_cw.md` (реализация Ethernet glue)
- `task_net.md` (использует link callbacks)
- `task_io.md` (управление LED на основе link state)
