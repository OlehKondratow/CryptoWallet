# CryptoWallet — документация

Единый технический канон (английский оригинал: [`../../README.md`](../../README.md)). Прошивка описана как **одна система**: границы доверия, поверхности, криптография, проверка и эксплуатация — на одном уровне детализации.

**Перевод:** этот каталог — русская версия; идентификаторы (имена функций, флаги сборки, пути к файлам) сохранены как в английском тексте.

| № | Документ | Содержание |
|---|----------|------------|
| 1 | [01-trust-model-and-architecture.md](01-trust-model-and-architecture.md) | Цепочка загрузки, границы доверия, угрозы, связь компонентов |
| 2 | [02-firmware-structure.md](02-firmware-structure.md) | Задачи, IPC, память, карта модулей → исходники |
| 3 | [03-cryptography-and-signing.md](03-cryptography-and-signing.md) | Ключи, trezor-crypto, подпись, TRNG и будущее хранение с привязкой к плате |
| 4 | [04-http-and-webusb.md](04-http-and-webusb.md) | HTTP по Ethernet, WebUSB, что **не** аутентифицировано |
| 5 | [05-uart-cwup-protocol.md](05-uart-cwup-protocol.md) | CWUP-0.1: фазы, команды, режимы RNG, статус реализации |
| 6 | [06-integrity-rng-verification.md](06-integrity-rng-verification.md) | `fw_integrity`, захват TRNG по UART, тесты на хосте, dieharder, CI |
| 7 | [07-build-ci-infrastructure.md](07-build-ci-infrastructure.md) | Флаги сборки, соседние репозитории / `CRYPTO_DEPS_ROOT`, Gitea, контейнеры |

**Генерируется автоматически (не править вручную):**

- `generated/reference-code.md` — из `@file` / `@brief` в исходниках (`make docs-code-md`)
- `generated/testing-plan-signing-rng.md` — из `scripts/test_plan_signing_rng.py`

**Инструменты:** [MAINTENANCE.md](MAINTENANCE.md) — MkDocs, Doxygen, опционально HTML (`site/`).

**Корень репозитория:** [../../../README.md](../../../README.md)
