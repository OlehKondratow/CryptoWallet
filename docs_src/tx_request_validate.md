\page tx_request_validate "tx_request_validate: validation gate for signing"
\related tx_request_validate
\related tx_recipient_format_ok
\related tx_amount_format_ok
\related tx_currency_supported

# `tx_request_validate.c` + `tx_request_validate.h`

<brief>Модуль `tx_request_validate` — охранный шлюз перед подписанием: он проверяет host-supplied поля (адрес получателя, сумму, валюту) на базовое соответствие формату, помогая избежать очевидно неправильных данных на SSD1306/в логе и защищая signing pipeline от груд ошибок.</brief>

## Краткий обзор
<brief>Модуль `tx_request_validate` — охранный шлюз перед подписанием: он проверяет host-supplied поля (адрес получателя, сумму, валюту) на базовое соответствие формату, помогая избежать очевидно неправильных данных на SSD1306/в логе и защищая signing pipeline от груд ошибок.</brief>

## Abstract (Synthèse логики)
Валидация "первой линии" перед всеми крипто-операциями: модуль отвечает за то, чтобы хост не смог подкинуть на подпись явно бредовые строки (пустые адреса, отрицательные суммы, неизвестные валюты). Это не "on-chain" валидация (которая — в протоколе блокчейна), а "санитария ввода" из сети/USB: гарантия, что UI покажет хоть что-то разумное и что `task_sign` не будет работать с NULL/overflow/garbage-значениями. Бизнес-роль — "fail fast" перед дорогой крипто-операцией.

## Logic Flow (validation gates)
Функция `tx_request_validate()` проходит последовательные проверки:
1. Null-check на сам объект.
2. **Recipient:** не пусто, длина < `TX_RECIPIENT_LEN`, формат OK (Base58 для P2PKH/P2SH или bech32 для SegWit).
3. **Amount:** не пусто, является ли десятичной строкой, содержит хотя бы одну цифру, одна точка макс, `atof` > 0.
4. **Currency:** если не пусто, то проверить против whitelist (BTC, ETH, LTC, BCH, DOGE, DASH, XMR); если пусто, дефолтировать на BTC.
5. На первой ошибке вернуть соответствующий enum `TX_VALID_ERR_*`.

Каждая проверка имеет свою хелпер-функцию (`tx_recipient_format_ok`, `tx_amount_format_ok`, `tx_currency_supported`).

## Прерывания/регистры
Нет ISR/регистров: модуль — чистая валидационная логика строк.

## Тайминги и условия ветвления
| Проверка | Условие отказа | Код ошибки |
|---|---|---|
| recipient empty | `recipient[0] == '\\0'` | `TX_VALID_ERR_RECIPIENT_EMPTY` |
| recipient too long | `len >= TX_RECIPIENT_LEN` | `TX_VALID_ERR_RECIPIENT_TOO_LONG` |
| recipient bad format | не Base58 (для '1'/'3') или не bech32 (для "bc1") | `TX_VALID_ERR_RECIPIENT_INVALID_CHARS` |
| amount empty | `amount[0] == '\\0'` | `TX_VALID_ERR_AMOUNT_EMPTY` |
| amount bad format | не decimal с цифрами / несколько точек / отрицательный знак не в начале | `TX_VALID_ERR_AMOUNT_INVALID` |
| amount negative | `atof(amount) < 0` | `TX_VALID_ERR_AMOUNT_NEGATIVE` |
| currency unsupported | не в whitelist при непустой строке | `TX_VALID_ERR_CURRENCY_UNSUPPORTED` |

## Dependencies
Прямые зависимости:
- Входная структура: `wallet_tx_t` (из `wallet_shared.h`).
- Выходной enum: `tx_validate_result_t` и функция `tx_validate_result_str()` для логирования.
- Стандартная библиотека: `string.h`, `stdlib.h`, `ctype.h`.

Косвенные:
- Используется в `task_net.c` (HTTP handler) перед формированием очереди.
- Используется в `task_sign.c` перед crpypto-path.
- Опционально в WebUSB handler.

## Связи
- `task_sign.md` (consumer с логированием ошибок)
- `task_net.md` / `app_ethernet_cw.md` (source хост-данных)
- `wallet_shared.md` (тип wallet_tx_t)
