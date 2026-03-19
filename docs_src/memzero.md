\page memzero "memzero: secure buffer zeroing"
\related memzero

# `memzero.c` + `memzero.h`

<brief>Модуль `memzero` вычищает sensitive buffer'ы (приватные ключи, digests, seeds) через volatile-writes, предотвращая оптимизацию компилятора и оставляя в памяти следы опасных данных.</brief>

## Краткий обзор
<brief>Модуль `memzero` вычищает sensitive buffer'ы (приватные ключи, digests, seeds) через volatile-writes, предотвращая оптимизацию компилятора и оставляя в памяти следы опасных данных.</brief>

## Abstract (Synthèse логики)
`memzero` — это микро-модуль, который решает одну критичную security проблему: современные компиляторы могут оптимизировать "бесполезный" `memset(buf, 0, len)` если компилятор заметит, что `buf` больше не используется. При работе с приватными ключами это опасно — данные остаются в памяти и могут быть извлечены при attack. Решение: использовать `volatile` указатель, который компилятор не может оптимизировать. Бизнес-роль — гарантировать, что на каждом exit-пути (успех, ошибка) приватные данные сразу же перезаписаны нулями.

## Logic Flow
Функция `memzero(void *pnt, size_t len)`:
1. Кастит указатель в `volatile unsigned char *`.
2. Цикл: byte-wise запись нулей через volatile указатель.
3. Выход.

Никакой state machine, никакой conditional logic — просто надёжная перезапись.

## Прерывания/регистры
Нет ISR/регистров: памяти-write операция на C-уровне.

## Тайминги и условия ветвления

| Параметр | Поведение |
|---|---|
| NULL указатель | no-op (len-- будет 0, сразу выход) |
| len = 0 | no-op (no iterations) |
| Любой buffer size | один байт за итерацию, через volatile |

## Dependencies
Нет зависимостей: `<stddef.h>` для `size_t`.

Используется из:
- `task_sign.c` — очистка seed/digest/sig после ошибок или успеха.
- `task_security.c` — то же в legacy FSM.
- `crypto_wallet.c` — очистка HDNode и приватного ключа.
- `wallet_seed.c` — очистка seed буфера.

## Связи
- `task_sign.md` (основной consumer)
- `crypto_wallet.md` (security в signing)
- `wallet_seed.md` (seed handling)
