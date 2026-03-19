# Учебная документация (`docs_src/`)

Здесь лежат **развёрнутые текстовые разборы** логики кода — дополнение к комментариям в исходниках и к выводу Doxygen (`make docs-doxygen`).

## Соглашение об именах

Один модуль (реализация + заголовок):

| Исходники | Markdown-гайд |
|-----------|----------------|
| `Core/Src/<module>.c` + `Core/Inc/<module>.h` | `docs_src/<module>.md` |

При необходимости для модулей вне `Core/` используется имя файла без пути, например `docs_src/task_net.md` для `Src/task_net.c`.

## Оглавление

### Инициализация и нижний уровень

| Документ | Описание |
|----------|---------|
| [hw_init.md](hw_init.md) | Board bring-up: часы, MPU/кэш (LwIP), GPIO, I2C/OLED, UART, USB/RNG |
| [memzero.md](memzero.md) | Secure buffer zeroing (защита от оптимизации компилятора) |

### Криптография

| Документ | Описание |
|----------|---------|
| [crypto_wallet.md](crypto_wallet.md) | trezor-crypto: RNG, BIP-39, BIP-32, ECDSA secp256k1 |
| [sha256_minimal.md](sha256_minimal.md) | Compact SHA-256 (fallback без trezor-crypto) |
| [wallet_seed.md](wallet_seed.md) | Test seed для development (USE_TEST_SEED=1) |

### Сетевой и HTTP слой

| Документ | Описание |
|----------|---------|
| [task_net.md](task_net.md) | LwIP + HTTP server (PORT 80): парсинг /tx, валидация, очередь подписания |
| [app_ethernet_cw.md](app_ethernet_cw.md) | Ethernet link FSM, DHCP, LED feedback |

### Валидация и подпись

| Документ | Описание |
|----------|---------|
| [tx_request_validate.md](tx_request_validate.md) | Validation gate: адрес, сумма, валюта перед signing |

### Задачи и пользовательский интерфейс

| Документ | Описание |
|----------|---------|
| [main.md](main.md) | Main entry point, FreeRTOS инициализация, task orchestration |
| [task_sign.md](task_sign.md) | Signing FSM: seed derivation, hash forming, ECDSA sign |
| [task_security.md](task_security.md) | Legacy signing FSM (test/audit) |
| [task_display.md](task_display.md) | SSD1306 UI: display queue, state merging, render functions |
| [task_display_minimal.md](task_display_minimal.md) | Minimal display (без full UI, для minimal-lwip builds) |
| [task_user.md](task_user.md) | User button: debouncing, confirm/reject events |
| [task_io.md](task_io.md) | LED indicator task |
| [time_service.md](time_service.md) | SNTP time sync, UTC formatting |

### Документация и API

| Документ | Описание |
|----------|---------|
| [wallet_shared.md](wallet_shared.md) | Shared IPC: queues, mutexes, events, structures |
| [doxygen-comments.md](doxygen-comments.md) | Стиль Doxygen комментариев в коде (@brief, @details) |
| [api-documentation-scope.md](api-documentation-scope.md) | Прогресс по документированию модулей |

## Как использовать

1. **Для разработчиков:** начните с [main.md](main.md), затем следуйте по ссылкам "Связи" в конце каждого документа.
2. **Для отладки:** найдите нужный модуль в таблице выше, откройте его `.md` и посмотрите раздел "Logic Flow".
3. **Для интеграции:** смотрите "Dependencies" каждого модуля, чтобы понять порядок инициализации.
4. **Для Doxygen:** `make docs-doxygen` сгенерирует HTML с перекрёстными ссылками между кодом и этими `.md` файлами.
