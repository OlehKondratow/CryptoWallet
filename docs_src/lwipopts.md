\page lwipopts "lwipopts: LwIP compile-time options (DHCP + SNTP)"

# `Core/Inc/lwipopts.h`

<brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP для проекта: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер heap’а (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, а также привязывает установку времени из SNTP к `time_service_set_epoch()`.</brief>

## Краткий обзор
<brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP для проекта: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер heap’а (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, а также привязывает установку времени из SNTP к `time_service_set_epoch()`.</brief>

## Abstract (Synthèse логики)
Встроенный стек — это не только “какие пакеты” отправляются, но и “какие ресурсы и структуры” доступны. `lwipopts.h` фиксирует эти ресурсы и поведение:
- какие подсистемы LwIP компилируются,
- где располагается heap в памяти,
- как устроены буферы PBUF и TCP окна,
- как происходит интеграция SNTP с приложением через `SNTP_SET_SYSTEM_TIME(sec)`.

Бизнес-задача config-файла — согласовать LwIP с ограничениями STM32H7 (память D2 SRAM и взаимодействие с кэшем/MPU) и с приложением `time_service`.

## Logic Flow (compile-time “state machine”)
Это не runtime state machine. Логика — в наборах макросов, которые определяют поведение подсистем:

| Группа параметров | Ключевые макросы | Что задают |
|---|---|---|
| UART alive logging | `LWIP_ALIVE_LOG` | печать “alive” с периодом в LwIP path |
| Heap/Memory | `MEM_ALIGNMENT`, `MEM_SIZE`, `LWIP_RAM_HEAP_POINTER` | размер/размещение кучи LwIP |
| Threading | `TCPIP_THREAD_*`, `DEFAULT_THREAD_STACKSIZE` | параметры internal TCP/IP thread’ов |
| Protocol enable | `LWIP_IPV4`, `LWIP_TCP`, `LWIP_DHCP`, `LWIP_SNTP`, `LWIP_DNS` | какие подсистемы компилируются |
| Timing integration | `SNTP_SET_SYSTEM_TIME(sec)` | вызывает `time_service_set_epoch(sec)` |
| Link callback | `LWIP_NETIF_LINK_CALLBACK` (через `LWIP_NO_LINK_THREAD`) | как LwIP сообщает изменения линка |

## Прерывания/регистры
Нет. Это конфигурация компиляции.

## Тайминги и критичные условия
| Параметр | Значение | Контекст |
|---|---:|---|
| `SNTP_UPDATE_DELAY` | `15*60*1000` мс | период обновления времени |
| `MEM_SIZE` | `14*1024` | влияет на возможность LwIP обслужить соединения и буферы |
| TCP буфер/окно | `TCP_SND_BUF`, `TCP_WND` | отражается на пропускной способности |

## Dependencies
Прямые:
- `time_service_set_epoch()` (декларация из `time_service`).

Косвенно:
- `hw_init` для корректного MPU/cache в ветке `USE_LWIP` (LwIP heap размещён в `0x30004000`, как часть MPU setup).

## Связи
- `hw_init.md`: почему heap/caches должны быть согласованы с LwIP.
- `time_service.md`: обработка SNTP epoch через `time_service_set_epoch`.
- `task_net.md` / `app_ethernet_cw.md`: runtime логика DHCP и старт SNTP.

