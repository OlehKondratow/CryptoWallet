\page wallet_seed "wallet_seed: test seed (development only)"
\related get_wallet_seed

# `wallet_seed.c`

<brief>Модуль `wallet_seed` — заглушка для `get_wallet_seed()` при `USE_TEST_SEED=1`: возвращает BIP-39 seed из известного тестового мnemonic'a "abandon...about", **никогда не для реальных средств**.</brief>

## Краткий обзор
<brief>Модуль `wallet_seed` — заглушка для `get_wallet_seed()` при `USE_TEST_SEED=1`: возвращает BIP-39 seed из известного тестового мnemonic'a "abandon...about", **никогда не для реальных средств**.</brief>

## Abstract (Synthèse логики)
Для обучения и тестирования нужен воспроизводимый seed. `wallet_seed` содержит жёсткозакодированный стандартный BIP-39 тестовый vector (адрес 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA), что позволяет воспроизводить подписи и проверять signing path. Это **development-only** — production надо заменить на secure element или encrypted flash. Бизнес-роль — обеспечить детерминированный seed при разработке.

## Logic Flow

1. При `USE_CRYPTO_SIGN=1` и `USE_TEST_SEED=1` компилируется реальная реализация.
2. Функция `get_wallet_seed()` просто вызывает `mnemonic_to_seed()` из trezor-crypto с известной строкой.
3. Копирует результат 64 байта в выходной буфер и очищает локальный seed через `memzero()`.
4. При любом другом флаге — компиляется пустой stub, и `get_wallet_seed()` (weak symbol в `task_sign.c`) просто возвращает -1.

## Прерывания/регистры
Нет ISR/регистров: просто строковое преобразование и memcpy.

## Тайминги и условия ветвления

| Сборка | Поведение |
|---|---|
| `USE_CRYPTO_SIGN=1` и `USE_TEST_SEED=1` | Возвращает seed из "abandon...about" |
| Любое другое | Weak stub: -1 (не реализовано) |

## Dependencies
- `bip39.h` из trezor-crypto (мnemonic_to_seed).
- `memzero.h` для очистки локального буфера.
- Используется из `task_sign.c` при подписании.

## Связи
- `task_sign.md` (consumer в signing pipeline)
- `crypto_wallet.md` (тезуры над trezor)
- `memzero.md` (seed cleanup)
