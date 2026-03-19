# Doxygen / API documentation scope

## Краткий обзор
Страница фиксирует прогресс по Doxygen-покрытию и показывает, какие модули уже получили “развёрнутый” комментарий (через `@brief/@details`).
Следующий шаг — превратить оставшиеся места в такие же учебные разборы без перегруза исходников.

## Детальный анализ

### What was expanded (this pass)

Full **@brief / @details** (and **@param / @return** where applicable) for:

- **`wallet_shared.h`** — structs, enums, macros, all `extern` IPC objects (queues, mutexes, events, signature buffer).
- **`tx_request_validate.h`** — enum values, all public validators.
- **`crypto_wallet.h`** — all public crypto APIs.
- **`memzero.h`**, **`sha256_minimal.h`**, **`hw_init.h`**, **`task_display.h`**, **`task_sign.h`**, **`task_user.h`**, **`task_io.h`**.
- **`task_sign.c`** — `get_wallet_seed`, `Task_Sign_Create`, `build_hash_input`, `sign_task`.
- **`task_user.c`**, **`task_io.c`**, **`task_display_minimal.c`**, **`tx_request_validate.c`** (static helpers), **`crypto_wallet.c`** / **`memzero.c`** (implementation `@copydoc` where useful).

### Remaining (optional next pass)

Large or boilerplate-heavy files can get the same treatment incrementally:

- **`task_display.c`** — `display_task`, `render_*`, queue handlers.
- **`Src/task_net.c`** — HTTP parser, static handlers.
- **`Src/app_ethernet_cw.c`** — DHCP / link callbacks.
- **USB stack** — `usb_webusb.c`, `usb_device.c`, descriptors.
- **HAL / Cube** — `stm32h7xx_it*.c`, `stm32h7xx_hal_msp.c` (short `@file` + IRQ role only is usually enough).

**C has no classes** — use **`typedef struct`** + field `/**< */` comments (as in `wallet_shared.h`) for “object-like” data.

Regenerate Doxygen + README table: `make docs-doxygen`.

## Связи
- Данные из Doxygen XML (`docs_doxygen/xml/`), который парсит `scripts/update_readme.py`.
- Правила оформления разборов: `docs_src/doxygen-comments.md`.
