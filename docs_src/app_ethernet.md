\page app_ethernet "app_ethernet: Ethernet glue interfaces (link callback + DHCP FSM)"

# `Core/Inc/app_ethernet.h`

<brief>Заголовок `app_ethernet` задаёт интерфейсы Ethernet “glue” слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`.</brief>

## Краткий обзор
<brief>Заголовок `app_ethernet` задаёт интерфейсы Ethernet “glue” слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`.</brief>

## Abstract (Synthèse логики)
Этот header — формальный контракт между LwIP и прикладным кодом. LwIP вызывает callback’и и ожидает, что приложение будет:
- переводить link-up/link-down события в понятные изменения состояния,
- управлять визуальными индикаторами,
- запускать и вести DHCP thread/FSM при необходимости.

С точки зрения “бизнес-логики” заголовок фиксирует, какие состояния DHCP существуют и через какие функции внешние модули могут взаимодействовать с Ethernet glue слоем.

## Logic Flow (contract + state model)
Run-time state machine реализована в `app_ethernet_cw.c`, но состояние формализовано здесь константами:
| Состояние | Значение | Смысл |
|---|---:|---|
| `DHCP_OFF` | 0 | DHCP не активен |
| `DHCP_START` | 1 | переход к старту DHCP после link-up |
| `DHCP_WAIT_ADDRESS` | 2 | ожидание lease/адреса |
| `DHCP_ADDRESS_ASSIGNED` | 3 | адрес получен |
| `DHCP_TIMEOUT` | 4 | DHCP не удался, выбирается static fallback |
| `DHCP_LINK_DOWN` | 5 | link упал, DHCP надо остановить/сбросить |

## Прерывания/регистры
Заголовок не содержит ISR и регистров — только объявления.

## Тайминги и условия ветвления
Условность подключения DHCP:
- все объявления `DHCP_Thread` компилируются только при `LWIP_DHCP`.

## Dependencies
Прямые:
- LwIP types: `struct netif` из `lwip/netif.h`.

Куда применяется:
- `app_ethernet_cw.c`: реализация интерфейса,
- `task_net.c`: регистрация link callback и старт DHCP thread.

## Связи
- `app_ethernet_cw.md`: реализует DHCP FSM и LED поведение.
- `task_net.md`: вызывает glue через callbacks и стартует системные потоки.

