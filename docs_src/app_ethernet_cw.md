\page app_ethernet_cw "app_ethernet_cw: link callbacks + DHCP LED FSM"
\related ethernet_link_status_updated
\related DHCP_Thread
\related time_service_start
\related ethernet_led2_is_on
\related ethernet_led3_is_on

# `Src/app_ethernet_cw.c` + `Core/Inc/app_ethernet.h`

<brief>Модуль `app_ethernet_cw` связывает LwIP с поведением платы: он реагирует на поднятие/падение Ethernet линка через callback, запускает DHCP или выбирает static fallback, и управляет индикаторами LED2/LED3, а также инициирует `time_service_start()` в момент, когда сеть готова (DHCP назначил адрес или сеть поднялась без DHCP).</brief>

## Краткий обзор
<brief>Модуль `app_ethernet_cw` связывает LwIP с поведением платы: он реагирует на поднятие/падение Ethernet линка через callback, запускает DHCP или выбирает static fallback, и управляет индикаторами LED2/LED3, а также инициирует `time_service_start()` в момент, когда сеть готова (DHCP назначил адрес или сеть поднялась без DHCP).</brief>

## Abstract (Synthèse логики)
Это “шов” между миром сетевого стека и миром пользователя/UX на микроконтроллере. Бизнес-задача — превратить абстрактные события LwIP (линк up/down, DHCP lease получен/не получен) в предсказуемое поведение LED-индикаторов и единственный момент запуска сервиса времени. Вся логика сведена к простым состояниям FSM, чтобы учесть непредсказуемые задержки DHCP и дрейф состояния линка.

## Logic Flow (Ethernet link + DHCP FSM)
### Событие линка: `ethernet_link_status_updated`
Алгоритм:
1. Считывается `link_up` через `netif_is_link_up(netif)`.
2. Если новое значение совпадает с прошлым (`s_last_link_up`) — выход без действий (дребезг/повторы игнорируются).
3. Если линк поднялся:
   - лог `"[ETH] Link up"`,
   - при `LWIP_DHCP` устанавливается `DHCP_state = DHCP_START`,
   - иначе: LED2=ON, LED3=OFF и сразу вызывается `time_service_start()`.
4. Если линк упал:
   - лог `"[ETH] Link down"`,
   - при `LWIP_DHCP` устанавливается `DHCP_state = DHCP_LINK_DOWN`,
   - иначе: LED2=OFF, LED3=ON.

Управление LED вынесено в `ethernet_set_led(led2_on, led3_on)`, при этом уровень учитывает актив-low полярность из `main.h`.

### DHCP FSM: `DHCP_Thread` (только если `LWIP_DHCP`)
FSM состояния (через `DHCP_state`, общий для callback и потока):
| Состояние | Входное событие/условие | Действия и выход |
|---|---|---|
| `DHCP_START` | линк поднялся | сброс IP/NETMASK/GW в netif->*, `ethernet_set_led(0,0)`, запуск `netifapi_dhcp_start(netif)`, переход к `DHCP_WAIT_ADDRESS` |
| `DHCP_WAIT_ADDRESS` | периодический опрос | если `dhcp_supplied_address(netif)` -> `DHCP_ADDRESS_ASSIGNED`, лог IP/MASK/GW, `time_service_start()`, LED2=ON/LED3=OFF; иначе если `dhcp->tries > MAX_DHCP_TRIES` -> `DHCP_TIMEOUT`, ставим static адрес (из макросов `main.h`), логируем, `time_service_start()`, LED2=ON/LED3=OFF |
| `DHCP_LINK_DOWN` | линк упал | `DHCP_state=DHCP_OFF`, `netifapi_dhcp_stop(netif)`, LED2=OFF/LED3=ON |

Период потока:
- `osDelay(500)` в конце каждой итерации, то есть реакция на изменения DHCP-state идёт “с грубой дискретизацией” 0.5s.

Константа:
| Параметр | Значение | Роль |
|---|---:|---|
| `MAX_DHCP_TRIES` | 12 | ограничитель попыток DHCP перед static fallback; комментарий подчёркивает, что 4 было слишком мало |
| стек DHCP thread | задаётся в `task_net.c` как `1024` | иначе DHCP thread мог бы не вместиться |

## Прерывания/регистры
Прямой работы с регистрами нет.
Есть взаимодействие с callback’ами LwIP/нетиф:
- `ethernet_link_status_updated()` вызывается из LwIP контекста link change,
- а DHCP работает как отдельный системный поток.

Корректность обеспечивается тем, что LED-состояния и `DHCP_state` — это простые shared переменные (в коде `DHCP_state` глобальный `uint8_t`).

## Тайминги и условия ветвления
| Условие | Тайминг | Последствие |
|---|---:|---|
| DHCP polling | 500ms | максимальная задержка реакции FSM примерно в этот интервал |
| DHCP timeout -> static | через `dhcp->tries > MAX_DHCP_TRIES` | выбирается static fallback вместо бесконечного DHCP ожидания |
| link_up change | по факту callback | при каждом изменении нового значения callback применяет LED/UI и переводит FSM |

## Dependencies
Прямые зависимости:
- LwIP: `lwip/netif.h`, `lwip/ip4_addr.h`, `lwip/netifapi.h`
- Взаимодействие с DHCP: `lwip/dhcp.h` (если `LWIP_DHCP`)
- UI/логирование: `Task_Display_Log()`
- Сервер времени: `time_service_start()`
- LED полярности: `LED2_*`, `LED3_*` из `main.h`

Глобальные структуры/флаги:
- `DHCP_state` (compile-time зависимость от `LWIP_DHCP`)
- `s_led2_on`, `s_led3_on` — текущее визуальное состояние
- `s_last_link_up` — защита от повторного выполнения на одинаковых событиях

## Связи
- `task_net.md`: стартует link thread/ DHCP thread и делает link callback registration.
- `time_service.md`: запускает SNTP после готовности сети.
- `task_display.md` / `task_display_minimal.md`: в момент событий пишет UI-лог и “DHCP…” строки.

