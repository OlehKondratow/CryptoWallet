\page lwipopts "lwipopts: LwIP compile-time options (DHCP + SNTP)"

# `Core/Inc/lwipopts.h`

<brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер кучи (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, привязывает обновления SNTP к `time_service_set_epoch()`.</brief>

## Обзор

<brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер кучи (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, привязывает обновления SNTP к `time_service_set_epoch()`.</brief>

## Абстракция (синтез логики)

Встроенный сетевой стек — это не только "какие пакеты отправляются", но и "какие ресурсы и структуры доступны". `lwipopts.h` фиксирует эти ресурсы и поведение:

- Какие подсистемы LwIP компилируются
- Где располагается куча в памяти
- Как устроены буферы PBUF и TCP окна
- Как происходит интеграция SNTP с приложением через `SNTP_SET_SYSTEM_TIME(sec)`

Бизнес-задача конфига: согласовать LwIP с ограничениями STM32H7 (память D2 SRAM и взаимодействие с кэшем/MPU) и с приложением `time_service`.

## Поток логики (compile-time "состояние")

Это не runtime конечный автомат. Логика — в наборах макросов, определяющих поведение подсистем:

| Группа параметров | Ключевые макросы | Назначение |
|---|---|---|
| UART alive логирование | `LWIP_ALIVE_LOG` | Печать "alive" периодически в LwIP пути |
| Куча/Память | `MEM_ALIGNMENT`, `MEM_SIZE`, `LWIP_RAM_HEAP_POINTER` | Размер и размещение кучи |
| Threading | `TCPIP_THREAD_*`, `DEFAULT_THREAD_STACKSIZE` | Параметры internal TCP/IP потоков |
| Включение протоколов | `LWIP_IPV4`, `LWIP_TCP`, `LWIP_DHCP`, `LWIP_SNTP`, `LWIP_DNS` | Какие подсистемы компилируются |
| Интеграция времени | `SNTP_SET_SYSTEM_TIME(sec)` | Вызывает `time_service_set_epoch(sec)` |
| Callback ссылки | `LWIP_NETIF_LINK_CALLBACK` (через `LWIP_NO_LINK_THREAD`) | Как LwIP сообщает об изменениях ссылки |

## Прерывания/регистры

Нет. Это конфигурация компиляции.

## Времена и критичные условия

| Параметр | Значение | Контекст |
|---|---|---|
| `SNTP_UPDATE_DELAY` | `15*60*1000` мс | Период обновления времени |
| `MEM_SIZE` | `14*1024` байт | Влияет на возможность LwIP обслужить соединения и буферы |
| TCP буфер/окно | `TCP_SND_BUF`, `TCP_WND` | Отражается на пропускной способности |

## Зависимости

Прямые:
- `time_service_set_epoch()` из `time_service.h`

Косвенно:
- `hw_init` для правильного MPU/кеша в ветке `USE_LWIP` (куча LwIP размещена в `0x30004000` как часть настройки MPU)

## Связи

- `hw_init.md` — Почему куча/кеши должны согласовываться с LwIP
- `time_service.md` — Обработка SNTP epoch через `time_service_set_epoch`
- `task_net.md` / `app_ethernet_cw.md` — Runtime логика DHCP и старт SNTP
