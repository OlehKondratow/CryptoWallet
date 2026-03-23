# 6. Целостность, RNG и проверка

## 6.1 Целостность прошивки в рантайме (`fw_integrity`)

**Назначение:** сравнить CRC32 и длину **исполняемой** области приложения с ожидаемыми — выявить повреждение или неверный образ. **Не** замена verified boot.

| Элемент | Где |
|---------|-----|
| Реализация | `Core/Src/fw_integrity.c`, `Core/Inc/fw_integrity.h` |
| Область | Символы линкера `__app_flash_start` / `__app_flash_end` |
| Строка в логе | `FWINFO crc32=…,len=…` при старте |
| Проверка на хосте | `scripts/fw_integrity_check.py` для `build/cryptowallet.bin`; опционально `--expect-fwinfo-line` из UART |

**CWUP:** `AT+FWINFO?` возвращает ту же строку при активном CWUP (`USE_RNG_DUMP=0`).

## 6.2 Два режима TRNG по UART (не смешивать)

| Режим | Сборка | Содержимое UART |
|-------|--------|-----------------|
| Сырой дамп | `USE_RNG_DUMP=1` | Непрерывные бинарные байты TRNG — **CWUP отключён** |
| Фреймовый (спец.) | CWUP §7 | `AT+RNG=START` затем фреймы — в прошивке **пока не реализовано** |

`scripts/capture_rng_uart.py` захватывает **сырые** байты для анализа в стиле dieharder при `USE_RNG_DUMP=1`.

**Криптографическое использование vs этот захват:** дамп по UART нужен, чтобы **статистически проверить** TRNG. **Запись** будущего зашифрованного блоба кошелька (соли, nonce AEAD) должна использовать **тот же аппаратный RNG внутри прошивки**, а не файл на хосте от capture — см. **раздел 3.5** [`03-cryptography-and-signing.md`](03-cryptography-and-signing.md) (роль TRNG, привязка UID, область `USE_RNG_DUMP`).

## 6.3 Тесты на хосте (без платы)

- **`pytest tests/mvp`** — CRC/сборка, разбор FWINFO, логика подделки образа secure boot (`ecdsa` из `requirements-test.txt`).
- **`scripts/fw_integrity_check.py`** — шаг CI «FW integrity».

## 6.4 Плата / CI

- **Gitea:** `.gitea/workflows/simple-ci.yml` — сборка, прошивка, анализ UART, **захват RNG** при `CI_BUILD_USE_RNG_DUMP=1` и если не пропущено.
- **Переменные:** `CRYPTO_DEPS_ROOT` (соседи `STM32CubeH7`, `stm32_secure_boot`, `stm32-ssd1306`), `CI_RNG_CAPTURE_BYTES` (по умолчанию большой захват для статистики), `CI_RNG_UART_CAPTURE_STRICT=0|1` (валит ли задачу неудачный захват), `CI_SKIP_RNG_UART_CAPTURE`.

## 6.5 Dieharder (быстрый смоук)

CI может запускать `dieharder` по захваченному файлу. **Код выхода 0** — норма даже если в одной строке **`FAILED`** — это одна статистика, не падение процесса. Короткие файлы **мотаются** много раз; **p-value нельзя трактовать как у бесконечного i.i.d. потока**. Для серьёзного разбора: большой захват (сотни МиБ) и расширенный dieharder офлайн.

## 6.6 Ручной чеклист

1. Сборка → `build/cryptowallet.bin`.
2. Прошивка → в логе UART есть `FWINFO`.
3. `python3 scripts/fw_integrity_check.py build/cryptowallet.bin` — сверить CRC/длину с логом устройства или `AT+FWINFO?` при необходимости.
