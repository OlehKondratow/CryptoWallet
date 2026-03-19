\page time_service "time_service: SNTP -> epoch + UTC форматирование"
\related time_service_init
\related time_service_start
\related time_service_now_epoch

# `time_service.c` + `time_service.h`

<brief>Модуль `time_service` обеспечивает синхронизацию времени по SNTP и даёт приложению унифицированный доступ к текущему Unix epoch и строковому представлению UTC (для логов/UI), построенному поверх `HAL_GetTick()` после получения epoch из сети.</brief>

## Краткий обзор

Модуль `time_service` обеспечивает синхронизацию времени по SNTP и даёт приложению унифицированный доступ к текущему Unix epoch и строковому представлению UTC (для логов/UI), построенному поверх `HAL_GetTick()` после получения epoch из сети.

## Логика потока (SNTP lifecycle)

Жизненный цикл:
1. Инициализация: сброс флагов и базовых значений; фиксируется текущий `HAL_GetTick()` как стартовая опора
2. Старт SNTP клиента: модуль запускается только один раз (guard `s_sntp_started`), инициализация через `tcpip_callback`, чтобы код жил в контексте LwIP
3. Получение epoch: при синхронизации сохраняются `s_epoch_base` и `s_tick_base_ms`, устанавливается `s_time_synced`
4. В любой момент:
   - вычисляется "now epoch" как `epoch_base + (HAL_GetTick()-tick_base)/1000`
   - форматируется строка UTC (если `s_time_synced`, иначе печатается `UNSYNCED`)

### Алгоритм epoch → UTC

Конвертация без RTC: деление на дни/секунды и последовательное вычитание дней по годам/месяцам с учётом високосных лет.

| Этап | Как считается |
|------|---|
| Разложение | epoch_sec → дни + rem → hour/minute/second |
| Годы | вычитание `365/366` пока days не войдёт в текущий год |
| Месяцы | вычитание дней по таблице месяцев, корректируя февраль для високосности |

## Прерывания и регистры

Регистров напрямую нет. Но есть асинхронность:
- старт SNTP инициируется через `tcpip_callback` (callback выполнится в контексте LwIP TCP/IP thread)
- расчёт "now epoch" завязан на `HAL_GetTick()`, то есть на системный tick/таймер HAL

## Тайминги и условия ветвления

| Guard/Флаг | Условие | Поведение |
|---|---|---|
| `s_sntp_started` | повторный вызов start | старт игнорируется |
| `s_time_synced` | запрос строки/времени | `time_service_now_string` отдаёт `UNSYNCED`, epoch-вычисление базируется на last epoch_base |

Критичная точка:
- `time_service_start()` возвращает "успех" только по факту постановки callback в `tcpip_callback`; сетевой результат приходит позже

## Зависимости

Прямые:
- LwIP: `lwip/apps/sntp.h`, `lwip/tcpip.h` (SNTP init + запуск в TCP/IP thread)
- Время/тайм-база: `HAL_GetTick()`
- UI/логирование: `Task_Display_Log()` при старте/ошибках/синхронизации

Глобальные структуры/флаги:
- локальные статические: `s_sntp_started`, `s_time_synced`, `s_epoch_base`, `s_tick_base_ms`

## Связи модулей

- `main.md`: стартует сервис init и инициирует bootstrap
- `task_net.md` / `app_ethernet_cw.md`: обычно отвечают за момент "после линка" (когда вызвать `time_service_start()`)
- `task_display.md`: показывает временные строки/хвосты логов
