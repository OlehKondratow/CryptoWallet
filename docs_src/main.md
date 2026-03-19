\page main "main: RTOS bootstrap + task wiring"
\related Error_Handler
\related HW_Init
\related HW_Init_Early_LwIP
\related Task_Display_Create
\related Task_Net_Create
\related Task_Sign_Create
\related Task_IO_Create
\related Task_User_Create

# `main.c` + `main.h`

<brief>Модуль `main` описывает “склейку” всего приложения на STM32H7: он задаёт порядок ранних HAL/LwIP инициализаций, создаёт IPC-объекты (очереди/семафоры/event group), запускает критические задачи FreeRTOS и поднимает планировщик.</brief>

## Краткий обзор
<brief>Модуль `main` описывает “склейку” всего приложения на STM32H7: он задаёт порядок ранних HAL/LwIP инициализаций, создаёт IPC-объекты (очереди/семафоры/event group), запускает критические задачи FreeRTOS и поднимает планировщик.</brief>

## Abstract (Synthèse логики)
`main` — это точка входа, которая превращает набор аппаратных блоков (такты, периферия) и прикладных задач (display/net/sign/io/user) в работающую систему. Вся “бизнес-логика” приложения фактически разнесена по задачам, но `main` определяет контракт по данным: какие глобальные структуры используются как каналы передачи событий, какие mutex’ы защищают общий ресурс (I2C/UI), и какие стартовые флаги подхватываются следующими уровнями (например, время/кэш при LwIP).

### Инварианты (что важно для корректности)
1. До `HAL_Init()` выполняется ранняя ветка, согласованная с LwIP (MPU/кэш).
2. До создания задач создаются IPC-объекты и mutex’ы, чтобы задачи не работали с `NULL` дескрипторами.
3. Порядок инициализаций соответствует “эталонному” стилю LwIP/Cube (lwip_zero / lwip-uaid-SSD1306).

## Logic Flow (bootstrap flow)
Линейной state machine на уровне “состояний системы” нет, но есть последовательность фаз:
1. Аппаратная подготовка (обход исключения на невыравненный доступ для M7).
2. Пакет ранней конфигурации под LwIP (только если `USE_LWIP`).
3. Стандартная HAL инициализация и затем `HW_Init()` (GPIO/I2C/UART/USB/RNG).
4. Инициализация сервиса времени.
5. По флагу включение крипто-RNG.
6. В “продакшн-сценарии” (без `BOOT_TEST`) создание очередей/семафоров/event group и старт ядра.
7. Создание задач в фиксированном порядке (display -> net -> sign -> io -> user).
8. Запуск планировщика.

## Прерывания/регистры
В `main.c` прямой работы с периферийными регистрами немного: главный “низкоуровневый” шаг — правка регистра системного поведения:
| Что делается | Зачем |
|---|---|
| `SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk` | Разрешить невыравненный доступ там, где стек/структуры LwIP могут читать поля протоколов “не по выравниванию”. |

## Тайминги и условия ветвления
| Условие | Результат |
|---|---|
| `BOOT_TEST` | Вместо FreeRTOS выполняется бесконечный диагностический цикл с `HAL_Delay(500)`. |
| `SKIP_OLED` | Баннер на SSD1306 не рисуется (сам SSD1306 уже мог быть проинициализирован в `HW_Init()`). |
| `USE_CRYPTO_SIGN` | Поднимается `crypto_rng_init()` (если включена криптография). |

## Dependencies
Прямые связи по данным/вызовам:
- Аппаратные: `HW_Init_Early_LwIP()`, `HW_Init()`, `HAL_Init()`.
- Время: `time_service_init()`.
- IPC и общий контракт с задачами: `g_tx_queue`, `g_display_queue`, `g_user_event_group`, `g_i2c_mutex`, `g_ui_mutex`, `g_display_ctx_mutex` и `g_display_ctx`.
- Таски: `Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`.
- UI/логирование: `Task_Display_Log()` и косвенно `UART_Log()` (через слой display).

Глобальные структуры/флаги, которые создаются/инициализируются в `main`:
- Очереди: `g_tx_queue` (net -> sign), `g_display_queue` (tx snapshot -> display).
- Event group: `g_user_event_group` (user -> sign подтверждение/отклонение).
- Mutex’ы: защита I2C/UI/display context.
- Флаги/статусы: `g_security_alert` и `g_last_sig_ready` остаются в значениях по умолчанию (в дальнейшем управляются задачами).

## Связи
- Аппаратный слой: `hw_init.md`
- Экран/лог: `task_display.md`, `task_display_minimal.md`
- Сеть: `task_net.md`, `app_ethernet_cw.md`
- Подтверждение/безопасность: `task_user.md`, `task_sign.md`, `task_security.md`
- Время: `time_service.md`

