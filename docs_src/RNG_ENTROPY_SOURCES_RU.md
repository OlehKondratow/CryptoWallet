# Источники случайности (RNG) в CryptoWallet и сравнение с Trezor

## Краткий ответ

- **Отдельного «температурного хаотического генератора»** (как отдельного блока, который снимает показания датчика температуры и подаёт их в RNG) **в коде CryptoWallet нет**.
- Используется **аппаратный TRNG микроконтроллера STM32H7** (`RNG` / `HAL_RNG_GenerateRandomNumber`), как описано в **Reference Manual** на серию: генератор на основе **аналоговых источников шума** (в т.ч. джиттер PLL и др.), сертифицированный подход NIST SP 800-90B. Физически в таких схемах участвует **тепловой шум** в полупроводниках, но это **не** то же самое, что «взять значение температуры с сенсора и назвать его энтропией».
- **Trezor** в актуальной прошивке опирается на **аппаратный TRNG** чипа / SoC (`rng_fill_buffer` и далее ChaCha DRBG в слоях вроде `random_delays`) — **не** на отдельный «температурный хаотический генератор» как единственный источник. Защитные **случайные задержки** (`wait_random`) используют уже готовый CSPRNG, а не датчик температуры.

## Где что в CryptoWallet

| Место | Назначение |
|-------|------------|
| `Core/Src/crypto_wallet.c` → `random_buffer()` | Криптография (trezor-crypto): слова с **`HAL_RNG_GenerateRandomNumber`**, XOR с программным **пулом** (LCG-обновление `1664525U * pool + 1013904223U`). |
| `crypto_rng_init()` | Начальный `random_reseed()` из **`HAL_GetTick()`** + побитовые перемешивания — **не** АЦП температуры. |
| `Core/Src/rng_dump.c` (USE_RNG_DUMP=1) | Сырой поток для **dieharder**: чтение **`RNG->DR`**, вывод по UART без текста. |

Итог для **подписей / BIP32**: энтропия идёт из **HW RNG + смешивание в коде** (как в типичной схеме с trezor-crypto hook).

**Модель угроз и процесс (P2):** [RNG_THREAT_MODEL_P2_RU.md](RNG_THREAT_MODEL_P2_RU.md).

## Полная проверка RNG (рекомендуемая последовательность)

1. **Сборка с потоком на UART**  
   `make clean && make USE_RNG_DUMP=1 …` (при необходимости `USE_CRYPTO_SIGN=1` и зависимости — см. `TESTING_GUIDE_RNG_SIGNING.md`).

2. **Прошивка**  
   `make flash` или `st-flash --reset write build/cryptowallet.bin 0x08000000`.

3. **Захват** (объём для dieharder `-a` обычно **≥ 128 MiB**):  
   `python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728`

4. **Статистика**  
   `dieharder -a -g 201 -f rng.bin` (долго), или `python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin`.

5. **Одним шагом (лаборатория)**  
   `./scripts/run_full_rng_verification.sh` — захват + анализ + dieharder (см. `TESTING_GUIDE_RNG_SIGNING.md`).

6. **CI (Gitea)**  
   По умолчанию **`USE_RNG_DUMP=1`** в `.gitea/workflows/simple-ci.yml` — см. **[CI_PIPELINE_ru.md](CI_PIPELINE_ru.md)**.

## Сравнение с Trezor (концептуально)

| Аспект | CryptoWallet (этот репозиторий) | Trezor (общая схема) |
|--------|----------------------------------|----------------------|
| Базовый источник | STM32H7 **аппаратный RNG** | Аппаратный **TRNG** SoC / MCU |
| Пользовательский random | `random_buffer` hook + LCG-пул | `rng_fill_buffer` → DRBG (ChaCha и т.д.) |
| «Температура» | Не выделена как отдельный источник в коде | Не выделена как основной источник в публичной схеме |

Если нужна **дополнительная** энтропия с АЦП/температуры — это **отдельная доработка** (сбор шума с канала, оценка min-entropy, смешивание в пул) — в текущем дереве не реализовано как в Trezor One классических «chaos from sensor» схемах.

См. также: `TESTING_GUIDE_RNG_SIGNING.md`, `docs_src/trezor-crypto-integration.md`, `Core/Src/crypto_wallet.c`.

**Улучшения безопасности TRNG (приоритеты, P0–P2):** [RNG_SECURITY_IMPROVEMENTS_RU.md](RNG_SECURITY_IMPROVEMENTS_RU.md).
