\page sha256_minimal "sha256_minimal: compact SHA-256 (no trezor-crypto)"
\related sha256_minimal

# `sha256_minimal.c` + `sha256_minimal.h`

<brief>Модуль `sha256_minimal` — компактная реализация SHA-256 (public domain), используется при `USE_CRYPTO_SIGN=0`, когда trezor-crypto не линкуется, но нужно хешировать данные для UI/логирования.</brief>

## Краткий обзор
<brief>Модуль `sha256_minimal` — компактная реализация SHA-256 (public domain), используется при `USE_CRYPTO_SIGN=0`, когда trezor-crypto не линкуется, но нужно хешировать данные для UI/логирования.</brief>

## Abstract (Synthèse логики)
`sha256_minimal` — it-yourself реализация SHA-256, не зависимая ни от чего, позволяет минимальной build (без trezor-crypto) иметь хотя бы базовый хеширование для целей отладки и UI. Это **не** предназначено для production signing (т.к. нет ECDSA), а скорее для симуляции/обучения. Бизнес-роль — обеспечить fallback при разборке crypto-слоя.

## Logic Flow

1. **Инициализация:** стандартные H-значения SHA-256 в массив `state[8]`.
2. **Основной цикл:** обработка данных по 64-байтным блокам.
3. **Padding:** добавление флага 0x80, нулей и длины в битах (как per SHA-256 spec).
4. **Transform:** для каждого 64-байтного блока:
   - Развёртывание 16 слов в W[0..63] через SIG0/SIG1 преобразования.
   - 64 раунда с роторами и мажоритарной логикой (CH, MAJ, EP0, EP1).
5. **Финал:** вывод 32 байта state в big-endian.

## Прерывания/регистры
Нет ISR/регистров: чистые вычисления.

## Тайминги и условия ветвления

| Этап | Что происходит |
|---|---|
| Input | Может быть любое len (включая 0) |
| Padding | Если len > 56 после добавления флага, требуется доп. блок transform |
| Output | Всегда 32 байта в big-endian |

## Dependencies
- `<stdint.h>`, `<string.h>` (memcpy, memset).
- Используется из `crypto_wallet.c` при `USE_CRYPTO_SIGN=0`.

## Связи
- `crypto_wallet.md` (fallback provider)
- `task_sign.md` (consumer при минимальной сборке)
