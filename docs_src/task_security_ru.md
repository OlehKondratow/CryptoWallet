\page task_security "task_security: legacy/mock signing FSM"
\related Task_Security_Create

# `task_security.c` + `task_security.h`

<brief>Модуль `task_security` хранит "legacy/mock" вариант signing FSM для bring-up и сравнения: он не используется в основном пути (`main.c` не вызывает его), но реализует схожую автоматику подтверждения и подменяет криптографию на заглушки.</brief>

## Краткий обзор

Модуль `task_security` хранит "legacy/mock" вариант signing FSM для bring-up и сравнения: он не используется в основном пути (`main.c` не вызывает его), но реализует схожую автоматику подтверждения и подменяет криптографию на заглушки.

## Логика потока (Legacy FSM)

Внутренняя state machine реализована как:
1. Локальная переменная `state` стартует в `SIGNING_IDLE`
2. В режиме IDLE задача забирает payload из `g_tx_queue` (timeout 200ms) и переводит состояние в `SIGNING_RECEIVED`
3. Далее на каждом тике выполняется одна итерация `fsm_signing_step`, которая обрабатывает:
   - RECEIVED: выполняет mock-hash и переводит в WAIT_CONFIRM
   - WAIT_CONFIRM: ждёт event bits confirm/reject с таймаутом 30s
   - IN_PROGRESS: выполняет mock-sign и переводит в DONE (или ERROR)
4. Когда достигнуты терминальные состояния (DONE/REJECTED/ERROR), задача очищает tx и возвращается в IDLE

### Отображение состояния

В модуле предусмотрен helper "update signing on display context", который мапит состояние FSM в поля `g_display_ctx` (locked/valid/pending).

## Прерывания и регистры

ISR/reg-доступ отсутствует. Модуль использует:
- event group ожидание
- очередь payload
- мем-очистку `memzero()` для sensitive буферов на путях выхода

## Тайминги и ветвления

| Параметр | Значение |
|----------|----------|
| timeout на queue receive | 200ms |
| таймаут confirm/reject | 30000ms |
| обработка | по итерациям цикла (без строгого периодического sleep, кроме задержек, зависящих от очередей/ожиданий) |

Ключевое ветвление:
- reject/timeout → SIGNING_REJECTED (или возвращение через терминальное состояние)
- без reject → CONFIRM → SIGNING_IN_PROGRESS → SIGNING_DONE

## Зависимости

Прямые зависимости:
- `g_tx_queue` / `wallet_tx_t` (вход)
- `g_user_event_group` / `EVENT_USER_CONFIRMED/REJECTED` (UX signal)
- `g_display_ctx_mutex` и `g_display_ctx` (вывод статусов)
- `memzero()` (sanitization)
- Мок-крипто функции внутри модуля (не реальные крипто-периферии)

Из архитектурных особенностей:
- в `main.c` `Task_Security_Create()` не вызывается; поэтому модуль по умолчанию не влияет на production-путь

## Связи модулей

- `task_sign.md` (production FSM, реальная криптография)
- `task_user.md` (source event bits)
- `wallet_shared.md` (общий контракт состояний FSM + event bits)
- `task_display.md` (контекст/лог-слой отображения)
