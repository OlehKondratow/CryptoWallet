\page security_and_testing_ru "Безопасность и тестирование (сводка)"
\related CI_PIPELINE_ru
\related UART_PROTOCOL_MVP_ru
\related INFRASTRUCTURE
\related main_ru

# Безопасность и тестирование CryptoWallet — сводный документ

<brief>Единая точка входа по **модели доверия**, **целостности образа**, **UART-диагностике (CWUP)**, **RNG** и **CI**. Детали остаются в профильных файлах; здесь — структура, границы ответственности и ссылки.</brief>

---

## 1. Зачем этот документ

В репозитории безопасность и тесты описаны в нескольких местах (`CI_PIPELINE_*`, `RNG_*`, `UART_PROTOCOL_MVP_*`, `TESTING_GUIDE_*`, загрузчик `stm32_secure_boot`). Этот файл **сводит** цели и маршруты проверки без дублирования длинных инструкций.

**English:** [SECURITY_AND_TESTING_EN.md](SECURITY_AND_TESTING_EN.md)

---

## 2. Цепочка загрузки и что считается «защитой»

```
[STM32 ROM]  →  (опционально) [Проверяемый загрузчик @ stm32_secure_boot]  →  [Приложение CryptoWallet]
```

| Уровень | Где задаётся | Что проверяется |
|---------|----------------|-----------------|
| ROM / option bytes | ST, конфигурация платы | Доступ к отладке, RDP, загрузочные векторы |
| Secure / verified bootloader | Репозиторий **`stm32_secure_boot`** (отдельно от CryptoWallet) | Подпись или иная аутентификация образа приложения до запуска |
| Приложение CryptoWallet | Этот репозиторий | Логика кошелька, **опционально** самопроверка целостности уже прошитого образа в Flash |

- **CWUP** ([UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md)) работает **только после** старта приложения FreeRTOS.
- **Подпись и TrustZone в загрузчике** — не в CryptoWallet; при смене схемы линковки согласовывайте с проектом `stm32_secure_boot`.

**Про «сертификат» bootloader:** в репозитории **`stm32_secure_boot`** подпись образа приложения делается **ECDSA secp256k1** и PEM-ключом (`scripts/sign_image.py`) — это **не** цепочка X.509/PKCS#7. Проверка формата на ПК и тесты **микропорчи** (один байт в теле приложения, в подписи, неверный magic): **`scripts/secure_boot_image.py`**, **`tests/mvp/test_secure_boot_sign_tamper.py`** (нужен `ecdsa` из `requirements-test.txt`). Отдельные образы **bootloader** vs **app** в этом тесте не прошиваются — проверяется только **логика подписанного app-образа** как в `sign_image.py`.

Команда **`AT+BOOTCHAIN?`** (CWUP) задумана как **лабораторная** выдача того, что прошивка «знает» о цепочке (макросы сборки, флаги). Это **не** эквивалент аппаратной проверки в ROM. Реализация UART: **`Core/Src/cwup_uart.c`** (не собирается в режиме приёма команд при **`USE_RNG_DUMP=1`**). Стресс очереди CWUP: **`scripts/test_cwup_mvp.py --stress-extra-rounds N`**.

---

## 3. Целостность образа приложения в рантайме (`fw_integrity`)

**Назначение:** обнаружить случайную порчу Flash, неверную прошивку или несоответствие собранному `build/cryptowallet.bin`. Это **не** замена verified boot.

| Компонент | Путь |
|-----------|------|
| Реализация | `Core/Src/fw_integrity.c`, `Core/Inc/fw_integrity.h` |
| Границы образа | символы линкера `__app_flash_start` / `__app_flash_end` в `STM32H743ZITx_FLASH_LWIP.ld` |
| Старт | `fw_integrity_init()` после `HW_Init()` в `main.c`; в лог — строка вида `FWINFO crc32=0x...,len=...,start=0x...,end=0x...` |
| Сверка на хосте | `scripts/fw_integrity_check.py` для `build/cryptowallet.bin` (опционально `--expect-crc` или **`--expect-fwinfo-line`** с полной строкой из `AT+FWINFO?` / лога — проверка CRC и **длины** файла) |

**Сопоставление с `.bin`:** CRC и длина относятся к **непрерывному диапазону** Flash приложения. Файл `cryptowallet.bin` от `objcopy` должен соответствовать тому же образу; если хвост сектора заполнен `0xFF`, это уже отражено в границах `end` — при расхождениях смотрите параметры `objcopy` и линковку. Подробнее о том, когда CRC совпадает с устройством, — в docstring **`scripts/fw_integrity_check.py`**.

**CWUP:** команда **`AT+FWINFO?`** возвращает ту же строку, что и лог **`FWINFO`** (`fw_integrity_snprint()`), если CWUP включён (сборка без `USE_RNG_DUMP`).

---

## 4. UART (CWUP) и тестирование защиты/целостности

Протокол: **[UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md)**.

**Таблица статуса команд** (что реализовано в прошивке, а что только в спецификации): **[UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md) §10**.

| Задача | Команда / действие |
|--------|---------------------|
| Версия протокола и сборки | `AT+CWINFO?`, после старта — строка `CW+READY,...` |
| Маркеры загрузки для CI | `AT+MARKS` — в прошивке **пока нет** |
| Целостность прошитого приложения | `AT+FWINFO?` или строка `FWINFO` в логе + `fw_integrity_check.py` |
| Понимание цепочки загрузки (лаб.) | `AT+BOOTCHAIN?` + документация `stm32_secure_boot` |
| Автотесты на ПК (без платы) | `pytest tests/mvp` — CRC, парсинг FWINFO/BOOTCHAIN, подпись `sign_image` + микропорча (`secure_boot_image.py`, нужен `ecdsa`) |
| HIL: CWUP по UART | `scripts/test_cwup_mvp.py` — `AT+BOOTCHAIN?`, стресс: `--stress-extra-rounds N`; опц. `--bin`; без платы: `CWUP_SKIP_NO_DEVICE=1` |

**Ограничение:** при **`USE_RNG_DUMP=1`** CWUP не активируется (тот же UART занят бинарным TRNG).

**Два режима TRNG по UART (не смешивать):** сборка с **`USE_RNG_DUMP=1`** выводит **сырой** непрерывный поток TRNG на UART3 и **отключает** текстовый CWUP. **Кадрированный** поток по спецификации CWUP (§7, команды **`AT+RNG=START`** / **`AT+RNG=STOP`**, ответ **`CW+RNG OPEN`**) — **отдельная** задуманная фича и **пока не реализована** в `Core/Src/cwup_uart.c`; это не то же самое, что «сырой» дамп.

---

## 5. RNG, энтропия и подпись

| Тема | Документ |
|------|----------|
| Рекомендации по TRNG, DRBG, fail-closed | [RNG_SECURITY_IMPROVEMENTS_RU.md](RNG_SECURITY_IMPROVEMENTS_RU.md) |
| Модель угроз P2, roadmap | [RNG_THREAT_MODEL_P2_RU.md](RNG_THREAT_MODEL_P2_RU.md) |
| Источники энтропии | [RNG_ENTROPY_SOURCES_RU.md](RNG_ENTROPY_SOURCES_RU.md) |
| Комплексные тесты RNG и подписи | [TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md) |
| Скрипт | `scripts/test_rng_signing_comprehensive.py` |

---

## 6. HTTP API (подпись по сети)

Спецификация маршрутов и форматов: **[HTTP_API_ru.md](HTTP_API_ru.md)** (реализация в `Src/task_net.c`).

---

## 7. CI и инфраструктура

| Тема | Документ / файл |
|------|-----------------|
| Pipeline, TRNG на UART, матрица флагов | [CI_PIPELINE_ru.md](CI_PIPELINE_ru.md), `.gitea/workflows/simple-ci.yml` |
| Железо runner, зависимости | [INFRASTRUCTURE.md](INFRASTRUCTURE.md) |
| Установка тестовых зависимостей | [INSTALL_TEST_DEPS.md](INSTALL_TEST_DEPS.md) |

---

## 8. Быстрая таблица «что проверить вручную»

1. Собрать прошивку → `build/cryptowallet.bin`.
2. Прошить устройство → в UART-логе найти строку **`FWINFO`**.
3. Выполнить `python3 scripts/fw_integrity_check.py build/cryptowallet.bin` и сравнить CRC/диапазон с логом.
4. (Опционально) прогнать RNG/подпись по [TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md).

---

## 9. История и сопутствующие материалы

- Задачи безопасности верхнего уровня (чек-листы): [crypto/task_security_ru.md](crypto/task_security_ru.md).
- Улучшения RNG не отменяют этот сводный документ — на них есть **перекрёстная ссылка** в начале [RNG_SECURITY_IMPROVEMENTS_RU.md](RNG_SECURITY_IMPROVEMENTS_RU.md).
