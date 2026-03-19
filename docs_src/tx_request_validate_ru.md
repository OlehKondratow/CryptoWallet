\page tx_request_validate "tx_request_validate: охранный шлюз перед подписанием"
\related tx_request_validate
\related tx_recipient_format_ok
\related tx_amount_format_ok
\related tx_currency_supported

# `tx_request_validate.c` + `tx_request_validate.h`

<brief>Модуль `tx_request_validate` — охранный шлюз перед подписанием: он проверяет host-supplied поля (адрес получателя, сумму, валюту) на базовое соответствие формату, помогая избежать очевидно неправильных данных на SSD1306/в логе и защищая signing pipeline от груд ошибок.</brief>

## Краткий обзор

Модуль `tx_request_validate` — охранный шлюз перед подписанием: он проверяет host-supplied поля (адрес получателя, сумму, валюту) на базовое соответствие формату, помогая избежать очевидно неправильных данных на SSD1306/в логе и защищая signing pipeline от груд ошибок.

## Логика потока (validation gates)

Функция `tx_request_validate()` проходит последовательные проверки:
1. Null-check на сам объект
2. **Recipient:** не пусто, длина < `TX_RECIPIENT_LEN`, формат OK (Base58 для P2PKH/P2SH или bech32 для SegWit)
3. **Amount:** не пусто, является ли десятичной строкой, содержит хотя бы одну цифру, одна точка макс, `atof` > 0
4. **Currency:** если не пусто, то проверить против whitelist (BTC, ETH, LTC, BCH, DOGE, DASH, XMR); если пусто, дефолтировать на BTC
5. На первой ошибке вернуть соответствующий enum `TX_VALID_ERR_*`

Каждая проверка имеет свою хелпер-функцию (`tx_recipient_format_ok`, `tx_amount_format_ok`, `tx_currency_supported`).

## Прерывания и регистры

Нет ISR/регистров: модуль — чистая валидационная логика строк.

## Тайминги и условия ветвления

| Проверка | Условие отказа | Код ошибки |
|----------|---|---|
| recipient empty | `recipient[0] == '\0'` | `TX_VALID_ERR_RECIPIENT_EMPTY` |
| recipient too long | `len >= TX_RECIPIENT_LEN` | `TX_VALID_ERR_RECIPIENT_TOO_LONG` |
| recipient bad format | не Base58 (для '1'/'3') или не bech32 (для "bc1") | `TX_VALID_ERR_RECIPIENT_INVALID_CHARS` |
| amount empty | `amount[0] == '\0'` | `TX_VALID_ERR_AMOUNT_EMPTY` |
| amount not decimal | содержит non-digit/non-dot chars | `TX_VALID_ERR_AMOUNT_INVALID_CHARS` |
| amount zero/negative | `atof(amount) <= 0` | `TX_VALID_ERR_AMOUNT_ZERO` |
| currency unsupported | не в whitelist и не пусто | `TX_VALID_ERR_CURRENCY_UNSUPPORTED` |

## Зависимости

Прямые:
- Input: `wallet_tx_t` с полями recipient/amount/currency
- Output: `tx_validate_result_str()` для human-readable сообщений об ошибках
- Нет зависимостей от внешней крипто или hardware

Косвенные:
- `task_net.md` (вызывает валидацию на полученном HTTP request)
- `task_sign.md` (получает валидированный tx из очереди)

## Связи модулей

- `task_net.md` (запрашивает валидацию перед enqueue)
- `task_sign.md` (потребляет валидированные транзакции)
- `wallet_shared.md` (контракт данных транзакции)
