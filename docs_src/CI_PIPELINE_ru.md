\page ci_pipeline "CI: сборка, TRNG на UART, pipeline"
\related INFRASTRUCTURE
\related TESTING_GUIDE_RNG_SIGNING
\related security_and_testing_ru

# CI pipeline (Gitea Actions)

<brief>Репозиторий: `.gitea/workflows/simple-ci.yml`. По умолчанию прошивка собирается с **`USE_RNG_DUMP=1`**: на UART идёт **бинарный поток аппаратного TRNG** (STM32 RNG → дамп для dieharder). Текстовые маркеры загрузки `[WALLET]` в этом режиме **не проверяются** — шаг «Analyse UART Log» пропускает ожидание строк.</brief>

## Суть

| Режим | `CI_BUILD_USE_RNG_DUMP` | UART | Маркеры `uart_boot_markers.txt` | `hardware-test` RNG capture |
|--------|-------------------------|------|----------------------------------|-----------------------------|
| **По умолчанию (push/PR)** | `1` | Бинарный TRNG | Пропуск | Включён |
| **Ручной запуск** | `ci_build_use_rng_dump`: **1** или **0** | 1 → TRNG / 0 → текст | Только при **0** | При **1** — да; при **0** — автопропуск без таймаута |

- **Переключить на текстовые логи:** только **Run workflow** → `ci_build_use_rng_dump` = **0** (отладка маркеров; RNG capture тогда не ждёт бинарный поток).
- **Отключить захват:** `CI_SKIP_RNG_UART_CAPTURE=1` (env runner).
- **Сборка 0, на плате RNG-прошивка:** `CI_RNG_UART_CAPTURE_FORCE=1`.

## MVP-тесты в pipeline

| Шаг | Что делается |
|-----|----------------|
| **Build** | `pytest tests/mvp` (хост): CRC, парсинг CWUP, `secure_boot_image` / микропорча (ключ `stm32_secure_boot` на runner — через `STM32_SECURE_BOOT` / `CRYPTO_DEPS_ROOT`) |
| **Analyse UART Log** | При **`CI_BUILD_USE_RNG_DUMP=0`**: после маркеров — `scripts/test_cwup_mvp.py` с `--bin build/cryptowallet.bin` и стресс-раундами (`CI_CWUP_STRESS_ROUNDS`, по умолчанию 5). При **`USE_RNG_DUMP=1`** шаг CWUP **пропускается** (на UART нет CWUP). `CI_CWUP_SKIP_NO_DEVICE=1` (по умолчанию): нет `/dev/ttyACM0` → выход 0; для строгой проверки на плате: `CI_CWUP_SKIP_NO_DEVICE=0`. |

Переменные workflow: `CI_CWUP_STRESS_ROUNDS`, `CI_CWUP_READY_TIMEOUT_SEC`, `CI_CWUP_SKIP_NO_DEVICE` (см. `.gitea/workflows/simple-ci.yml`).

Отдельного workflow только под CWUP в репозитории нет: HIL на UART — через **тот же Simple CI** (ручной **Run workflow** с `ci_build_use_rng_dump=0` или шаг «Analyse UART Log», когда сборка в текстовом режиме).

## Почему в режиме RNG не ждут текстовые UART-маркеры

Это не «железная» необходимость, а **текущая модель прошивки и тестов**:

1. **Поток для dieharder должен быть только сырьём TRNG.** Любые строки логов в том же потоке — **загрязнение выборки** (статистика уже не про чистый RNG). В `Core/Src/rng_dump.c` задача непрерывно передаёт **бинарные** блоки на UART3; параллельно другие задачи могут писать логи на тот же UART — получается **смешение**, которое для dieharder нежелательно.

2. **Скрипт маркеров** (`uart_wait_boot_log.py`) ищет **читаемые подстроки** в логе. В потоке, где доминирует случайный байтовый шум, поиск строк **неинтерпретируем** (ложные совпадения / бесконечное ожидание).

3. **Совместить «как AT-модем» можно в принципе**, но это отдельный **протокол**: режимы команд / данных, escape-последовательности, синхронизация, на стороне ПК — конечный автомат. Сейчас такого слоя **нет** — есть только флаг сборки `USE_RNG_DUMP` и отдельные сценарии CI (маркеры **или** чистый захват RNG).

Полный гайд по тестам RNG/подписи: [TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md).  
Инфраструктура runner: [INFRASTRUCTURE.md](INFRASTRUCTURE.md) (детали железа, пути зависимостей).

**English:** [CI_PIPELINE_en.md](CI_PIPELINE_en.md)

**Протокол UART (MVP, команды + TRNG):** [UART_PROTOCOL_MVP_ru.md](UART_PROTOCOL_MVP_ru.md)

**Безопасность и тестирование (сводка):** [SECURITY_AND_TESTING_RU.md](SECURITY_AND_TESTING_RU.md)
