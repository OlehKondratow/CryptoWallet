\page task_sign "task_sign: конвейер подписания транзакций + FSM подтверждения"
\related Task_Sign_Create
\related g_tx_queue
\related g_user_event_group
\related g_last_sig_ready

# `task_sign.c` + `task_sign.h`

<brief>Модуль `task_sign` реализует основной pipeline подписания: он получает валидированный запрос из очереди, формирует детерминированный input для SHA-256, ждёт пользовательское подтверждение/отклонение и, при успехе, сохраняет компактную подпись и подготавливает её для сетевого/USB ответа.</brief>

## Краткий обзор

`task_sign` — центральная точка "политики и криптографии" в проекте: он соединяет данные от сети с подтверждением человека и криптографической операцией подписи. Его бизнес-роль — гарантировать, что криптооперации не стартуют до валидности входных строк и не завершаются без явного `Confirm` события от `task_user`. При ошибках он переводит систему в безопасный UX режим (через `g_security_alert` и обновление display context).

## Логика потока (явная FSM подписания)

Внутренняя логика оформлена через enum `signing_state_t` и локальную переменную `state`. Смысловые состояния и переходы:

### 1) SIGNING_IDLE

Шаги:
1. Ожидание транзакции из `g_tx_queue` (timeout 200ms).
2. Валидация host payload через `tx_request_validate`.
3. При ошибке валидации: логирование, выставление `g_security_alert=1`, очистка временных буферов и возврат в `IDLE`.
4. При успехе: переход в `SIGNING_RECEIVED`.

### 2) SIGNING_RECEIVED

Шаги:
1. Обновление display context (валюта/amount, безопасный locked=false пока идёт ожидание confirm).
2. Формирование детерминированного SHA-256 input из полей recipient/amount/currency.
3. Расчёт SHA-256 (ошибка → security_alert=1 и возврат в IDLE).
4. Переход в `SIGNING_WAIT_CONFIRM`.

### 3) SIGNING_WAIT_CONFIRM

Шаги:
1. Очистка event group битов `EVENT_USER_CONFIRMED | EVENT_USER_REJECTED`.
2. Ожидание одного из событий с таймаутом `CONFIRM_TIMEOUT_MS=30000ms`.
3. Ветвления:
   - `REJECTED` → лог "Rejected", очистка, возврат в IDLE
   - без `CONFIRMED` (timeout) → лог "Timeout", очистка, возврат в IDLE
4. На confirm:
   - получение seed (`get_wallet_seed`) и derive ключа m/44'/0'/0'/0/0
   - криптоподпись `crypto_sign_btc_hash(...)`
   - сохранение `g_last_sig` и установка `g_last_sig_ready=1`
   - обнуление буферов sensitive
   - сброс `g_security_alert=0`
   - обновление display context на locked/valid и очистка pending флага через `UI_ClearPending()`

## Прерывания и регистры

Прямых ISR/reg-уровневых операций нет. Однако есть критичный security аспект:
- Sensitive буферы очищаются через `memzero()` на всех путях выхода (включая ветви ошибок)
- Ключевые "точки" безопасного поведения завязаны на event group и очереди

## Тайминги и условия ветвления

| Момент | Значение | Что контролирует |
|--------|-------:|---|
| ожидание payload из очереди | 200ms | не блокировать навсегда при пустой очереди |
| ожидание mutex display context | 50ms | избегать зависаний при проблемах с UI mutex |
| подтверждение пользователя | 30000ms | timeout отказывает в подписании |
| очередь/merge display pending | через `UI_ClearPending()` | переход UX из confirm-режима |

## Зависимости

Прямые зависимости:
- Входные данные: `g_tx_queue` (`wallet_tx_t`)
- Пользовательский сигнал: `g_user_event_group` + биты `EVENT_USER_CONFIRMED/REJECTED`
- Валидация: `tx_request_validate()` и `tx_validate_result_str()`
- Крипто-слой: `crypto_hash_sha256()`, `crypto_derive_btc_m44_0_0_0_0()`, `crypto_sign_btc_hash()`
- Seed: `get_wallet_seed()` (weak stub в этом модуле; реальная реализация может быть в `wallet_seed.c` при тестовых сборках)
- UI/логирование: `Task_Display_Log()`, `UI_ClearPending()` и protected доступ к `g_display_ctx`

Глобальные структуры/флаги, которые меняются:
- `g_last_sig`, `g_last_sig_ready`
- `g_security_alert`
- поля `g_display_ctx` (currency/amount/safe_locked/signature_valid)

## Связи модулей

- `task_net.md` (создаёт/кладёт payload в `g_tx_queue`)
- `tx_request_validate.md` (валидация)
- `task_user.md` (confirm/reject event bits)
- `task_display.md` (рендер pending/UX)
- `crypto_wallet.md` / `memzero.md` (крипто и sanitization)
