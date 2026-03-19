\page task_security "task_security: legacy/mock signing FSM"
\related Task_Security_Create

# `task_security.c` + `task_security.h`

<brief>Модуль `task_security` хранит “legacy/mock” вариант signing FSM для bring-up и сравнения: он не используется в основном пути (`main.c` не вызывает его), но реализует схожую автоматику подтверждения и подменяет криптографию на заглушки.</brief>

## Краткий обзор
<brief>Модуль `task_security` хранит “legacy/mock” вариант signing FSM для bring-up и сравнения: он не используется в основном пути (`main.c` не вызывает его), но реализует схожую автоматику подтверждения и подменяет криптографию на заглушки.</brief>

## Abstract (Synthèse логики)
Это модуль для обучения и диагностики: вместо “реальной” криптографии он моделирует шаги подписи (SHA-256/ECDSA) и показывает, как может выглядеть FSM при ожидании confirm/reject. Бизнес-задача `task_security` — сохранить структуру потоков (queue -> ждём USER -> “подпись/отказ”) и при этом безопасно отображать состояние через `g_display_ctx`, не затрагивая основную production-схему `task_sign`.

## Logic Flow (legacy FSM)
Внутренняя state machine реализована как:
1. Локальная переменная `state` стартует в `SIGNING_IDLE`.
2. В режиме IDLE задача забирает payload из `g_tx_queue` (timeout 200ms) и переводит состояние в `SIGNING_RECEIVED`.
3. Далее на каждом тике выполняется одна итерация `fsm_signing_step`, которая обрабатывает:
   - RECEIVED: выполняет mock-hash и переводит в WAIT_CONFIRM,
   - WAIT_CONFIRM: ждёт event bits confirm/reject с таймаутом 30s,
   - IN_PROGRESS: выполняет mock-sign и переводит в DONE (или ERROR),
4. Когда достигнуты терминальные состояния (DONE/REJECTED/ERROR), задача очищает tx и возвращается в IDLE.

### Отображение состояния
В модуле предусмотрен helper “update signing on display context”, который мапит состояние FSM в поля `g_display_ctx` (locked/valid/pending).

## Прерывания/регистры
ISR/reg-доступ отсутствует. Модуль использует:
- event group ожидание,
- очередь payload,
- мем-очистку `memzero()` для sensitive буферов на путях выхода.

## Тайминги и ветвления
| Параметр | Значение |
|---:|---:|
| timeout на queue receive | 200ms |
| таймаут confirm/reject | 30000ms |
| обработка | по итерациям цикла (без строгого периодического sleep, кроме задержек, зависящих от очередей/ожиданий) |

Ключевое ветвление:
- reject/timeout -> SIGNING_REJECTED (или возвращение через терминальное состояние)
- reject absent -> CONFIRM -> SIGNING_IN_PROGRESS -> SIGNING_DONE

## Dependencies
Прямые зависимости:
- `g_tx_queue` / `wallet_tx_t` (вход)
- `g_user_event_group` / `EVENT_USER_CONFIRMED/REJECTED` (UX signal)
- `g_display_ctx_mutex` и `g_display_ctx` (вывод статусов)
- `memzero()` (sanitization)
- Мок-крипто функции внутри модуля (не реальные крипто-периферии).

Из архитектурных особенностей:
- в `main.c` `Task_Security_Create()` не вызывается; поэтому модуль по умолчанию не влияет на production-путь.

## Связи
- `task_sign.md` (production FSM, реальная криптография)
- `task_user.md` (source event bits)
- `wallet_shared.md` (общий контракт состояний FSM + event bits)
- `task_display.md` (контекст/лог-слой отображения)

