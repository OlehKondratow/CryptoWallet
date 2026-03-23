# План тестов: подпись Bitcoin и RNG

_Сгенерировано: см. английский `documentation/generated/testing-plan-signing-rng.md` — этот файл перевода может устареть после регенерации._

Этот план дополняет документацию проекта. Автоматические крючки минимальны; отмечайте пункты вручную в трекере.

## 1. Область

- Путь подписи Bitcoin в стиле m/44'/0'/0'/0/0 (по умолчанию в проекте).
- Подписывается сообщение: SHA-256(recipient|amount|currency), затем ECDSA secp256k1 компактная подпись 64 байта — **не** сырой Bitcoin tx.
- RNG: STM32 TRNG + программный пул (см. `documentation/03-cryptography-and-signing.md`).

## 2. Подпись Bitcoin — предпосылки

- Сборка: USE_CRYPTO_SIGN=1, USE_TEST_SEED=1 для известного вектора (только dev).
- Прошивка; Ethernet или WebUSB по `documentation/04-http-and-webusb.md`.
- UART 115200 для логов (желательно).

## 3. Подпись Bitcoin — функциональные тесты

- **T-SIG-01** POST /tx валидный JSON → UART: TX в очереди, TX recv; дисплей pending/sign.
- **T-SIG-02** Короткое USER (PC13) → Confirm → UART: Signed OK (с тестовым seed).
- **T-SIG-03** GET /tx/signed → JSON signed + hex sig 128 символов (64 байта).
- **T-SIG-04** Долгое USER → Reject → нет валидной подписи / Reject в логе.
- **T-SIG-05** Нет нажатия 30 с → таймаут.
- **T-SIG-06** Невалидный JSON / плохой получатель → TX invalid.
- **T-SIG-07** WebUSB: `scripts/test_usb_sign.py` ping затем sign (USE_WEBUSB=1).
- **T-SIG-08** Перекрёстная проверка: те же входы + тот же seed → воспроизводимая подпись (детерминированный k только при пути RFC6979; сверить с крипто проекта).

## 4. Подпись Bitcoin — безопасность / негатив

- **T-SEC-01** Без USE_TEST_SEED и без реализации get_wallet_seed → после подтверждения нет seed.
- **T-SEC-02** Нет приватного ключа или seed в ответах UART/HTTP (проверить логи).

## 5. RNG — проверки дизайна

- **T-RNG-01** HAL_RNG / TRNG включён в сборке; в продуктовой конфигурации нет пути только на fallback.
- **T-RNG-02** После сброса вывод random_buffer различается между циклами питания (выборочно).
- **T-RNG-03** Ревью кода: memzero на путях seed/privkey после использования (task_sign / crypto_wallet).

## 6. RNG — статистические тесты DIEHARDER

- Цель: выявить грубое смещение / корреляцию; не криптосертификация.
- **Прошивка:** только бинарный поток на UART (без текста). При необходимости USE_RNG_DUMP.
- **Файл захвата:** `scripts/capture_rng_uart.py --port ... --out rng.bin --bytes 134217728` (рекомендуется ≥128 МиБ).
- **Запуск:** `scripts/run_dieharder.py --file rng.bin` (нужен пакет `dieharder`).
- **Критерий:** интерпретировать p-value; исследовать скопления провалов; при пограничных — повторить с большим файлом.

## 7. Прослеживаемость

- Дизайн: `documentation/03-cryptography-and-signing.md`, `04-http-and-webusb.md`, `06-integrity-rng-verification.md`.

## Чеклист файла DIEHARDER

[ ] Прошивка выдаёт **только** сырые байты на выбранном интерфейсе.
[ ] Скорость UART совпадает со скриптом (по умолчанию 115200).
[ ] `scripts/capture_rng_uart.py` записал ожидаемое число байт.
[ ] `dieharder` установлен: работает `dieharder -l`.
[ ] `scripts/run_dieharder.py --file rng.bin` завершился; результаты заархивированы.
