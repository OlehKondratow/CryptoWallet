# 🧪 Comprehensive Testing Guide: RNG & Transaction Signing

**Файл:** `scripts/test_rng_signing_comprehensive.py`  
**Дата создания:** 2026-03-19  
**Версия:** 1.0  
**Статус:** ✅ Готов к использованию

---

## 📋 Оглавление

1. [Быстрый старт](#быстрый-старт)
2. [Требования](#требования)
3. [Сборка прошивки](#сборка-прошивки)
4. [Тестирование RNG](#тестирование-rng)
5. [Тестирование подписания](#тестирование-подписания)
6. [Полное тестирование](#полное-тестирование)
7. [Интерпретация результатов](#интерпретация-результатов)
8. [Troubleshooting](#troubleshooting)

---

## 🚀 Быстрый старт

### Вариант 1: Только RNG (128 MiB, ~30 мин)

```bash
# Сборка прошивки для RNG
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash

# Запуск тестирования
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0

# Результат: rng.bin (128 MiB)
```

### Вариант 2: Только подписание (с HTTP)

```bash
# Сборка прошивки для подписания
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4
make flash

# Запуск тестирования
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Результат: журнал проверок подписания
```

### Вариант 3: DIEHARDER на существующем файле

```bash
# Если уже есть rng.bin
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin

# Результат: статистические тесты (1-3 часа)
```

### Вариант 4: Полное тестирование (все)

```bash
# Всё: RNG + DIEHARDER + подписание
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

---

## ✅ Требования

### Обязательные

- ✅ Python 3.8+
- ✅ ARM toolchain (arm-none-eabi-gcc)
- ✅ STM32 programmer (st-flash или OpenOCD)

### Для RNG тестирования

- 🟡 `dieharder` — статистические тесты
  ```bash
  sudo apt install dieharder
  ```

- 🟡 `pyserial` — коммуникация UART
  ```bash
  pip install pyserial
  ```

### Для тестирования подписания

- 🟡 `requests` — HTTP клиент
  ```bash
  pip install requests
  ```

### Установка всех зависимостей

```bash
# Python
pip install pyserial requests

# System
sudo apt install dieharder
```

---

## 🔨 Сборка прошивки

### Для RNG тестирования (UART capture)

```bash
cd /data/projects/CryptoWallet

# Конфигурация для RNG UART output
make clean
make USE_CRYPTO_SIGN=1 \
      USE_TEST_SEED=1 \
      USE_RNG_DUMP=1 \
      -j4

# Прошивка на устройство
make flash

# Проверка размера
arm-none-eabi-size build/firmware.elf
# text    data     bss     dec     hex filename
# 156000   2500   12000  170500   29d3c firmware.elf

# Проверка интеграции
./scripts/check_integration.sh
```

**Что включается:**
- ✅ `USE_CRYPTO_SIGN=1` — полная криптография (trezor-crypto)
- ✅ `USE_TEST_SEED=1` — hardcoded seed для репродуцируемости
- ✅ `USE_RNG_DUMP=1` — выводит ТОЛЬКО raw bytes на UART

### Для тестирования подписания (HTTP)

```bash
make clean
make USE_CRYPTO_SIGN=1 \
      USE_TEST_SEED=1 \
      USE_LWIP=1 \
      -j4

make flash
```

**Что включается:**
- ✅ `USE_LWIP=1` — Ethernet + HTTP сервер
- ✅ HTTP `/tx` endpoint
- ✅ DHCP или static IP (192.168.0.10)

---

## 🎲 Тестирование RNG

### Этап 1: Подготовка

```bash
# Убедиться, что прошивка запущена с USE_RNG_DUMP=1
screen /dev/ttyACM0 115200
# Должны видеть: только сырые байты (бинарные), ничего текстового
# Выход: Ctrl-A, Ctrl-\
```

### Этап 2: Захват RNG данных

```bash
# По умолчанию (128 MiB)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# С кастомным портом
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyUSB0

# Большой файл (256 MiB)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --bytes 268435456

# С пропуском первых байт (если boot garbage)
# [В исходном коде: --skip 1000]
```

**Ожидаемый вывод:**

```
======================================================================
                     RNG DATA CAPTURE FROM UART
======================================================================

ℹ Port: /dev/ttyACM0, Baud: 115200
ℹ Target: 128 MiB
ℹ Make sure firmware has USE_RNG_DUMP=1

  8 MiB / 128 MiB (0.45 MB/s, ETA: 253s)
 16 MiB / 128 MiB (0.45 MB/s, ETA: 245s)
...
128 MiB / 128 MiB (0.45 MB/s, ETA: 0s)

✓ Captured 134217728 bytes to rng.bin
```

**Время:** ~30 мин @ 115200 baud

### Этап 3: Анализ RNG качества

```bash
# Быстрый анализ (entropy, chi-square)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --file rng.bin
```

**Ожидаемый вывод:**

```
======================================================================
                      RNG QUALITY ANALYSIS
======================================================================

ℹ File size: 128.0 MiB
ℹ Sample entropy: 7.9987 bits/byte (max: 8.0)
ℹ Chi-square p-value: 0.4523

✓ Entropy looks good
```

**Критерии:**
- ✅ Entropy > 7.5 bits/byte → хороший RNG
- ✅ Chi-square p-value между 0.001 и 0.999 → PASS
- ⚠️ Entropy < 7.5 → смещение (bias)

### Этап 4: Запуск Dieharder

```bash
# Все тесты (~3 часа)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Конкретный тест (test 1, subtest 0)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --test 1

# Список тестов
dieharder -l
```

**Ожидаемый вывод:**

```
======================================================================
                   DIEHARDER STATISTICAL TESTS
======================================================================

ℹ Input file: rng.bin (128.0 MiB)
ℹ Command: dieharder -g 201 -f rng.bin -a
ℹ Running tests (this may take 30 min - 3 hours)...

diehard_birthdays
  p-value = 0.523  PASS ✓
diehard_operm5
  p-value = 0.427  PASS ✓
diehard_rank_32x32
  p-value = 0.001  FAIL ✗
...

✓ DIEHARDER tests completed
```

**Интерпретация результатов:**
- ✅ 80-95% PASS → RNG в порядке
- ⚠️ 5-15% WEAK → граничный случай
- ❌ >20% FAIL → серьёзные проблемы

---

## 🔐 Тестирование подписания

### Этап 1: Подготовка устройства

```bash
# Проверка подключения (должно быть USE_LWIP=1)
# Устройство должно получить IP по DHCP или иметь static 192.168.0.10

# Проверка статуса
curl http://192.168.0.10/status

# Ожидаемый ответ:
# {"status":"ready","version":"1.0"}
```

### Этап 2: Запуск функциональных тестов

```bash
# По умолчанию (IP 192.168.0.10)
python3 scripts/test_rng_signing_comprehensive.py --mode signing

# С кастомным IP
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.1.100

# С кастомным портом [требует модификации скрипта]
```

**Ожидаемый вывод:**

```
======================================================================
                    TRANSACTION SIGNING TESTS
======================================================================

ℹ Checking device connectivity: http://192.168.0.10:80

  Testing: 0.1 BTC to 1LqBGSK...
    ✓ TX accepted
    ℹ Waiting for device confirmation (30s timeout)...
    ✓ Signature: ABCDEF123456...

  Testing: 0.5 ETH to 1A1z7ago...
    ✓ TX accepted
    ✓ Signature: 123456ABCDEF...
```

### Этап 3: Тест детерминизма (RFC6979)

```bash
# Проверка: одинаковые TX → одинаковые подписи
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
```

**Ожидаемый результат:**

```
======================================================================
              DETERMINISTIC SIGNING TEST (RFC6979)
======================================================================

ℹ Same TX should produce identical signatures...

  Attempt 1/3: Sending TX...
    ℹ Got signature: ABCDEF123456...

  Attempt 2/3: Sending TX...
    ℹ Got signature: ABCDEF123456...

  Attempt 3/3: Sending TX...
    ℹ Got signature: ABCDEF123456...

✓ Signatures are deterministic (RFC6979)
```

---

## 🧪 Полное тестирование (все системы)

```bash
# Автоматически запустит:
# 1. Проверка RNG (entropy analysis)
# 2. Захват RNG (128 MiB с UART)
# 3. DIEHARDER тесты (все)
# 4. Проверка подписания
# 5. Тест детерминизма

python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

**Ожидаемое время:** 3-5 часов

---

## 📊 Интерпретация результатов

### RNG Entropy

| Значение | Интерпретация | Действие |
|----------|---------------|---------|
| > 7.9 bits/byte | Отличный | ✅ OK |
| 7.5-7.9 bits/byte | Хороший | ✅ OK |
| 7.0-7.5 bits/byte | Приемлемый | ⚠️ Мониторить |
| < 7.0 bits/byte | Плохой | ❌ Проблема |

### Dieharder p-values

| p-value | Результат | Действие |
|---------|-----------|---------|
| 0.001 - 0.999 | PASS | ✅ OK |
| 0.0001 - 0.001 | WEAK | ⚠️ Переучить |
| < 0.0001 | FAIL | ❌ Исследовать |
| > 0.9999 | FAIL | ❌ Исследовать |

### Подписание

| Результат | Интерпретация |
|-----------|---------------|
| Все TX подписаны | ✅ OK |
| Некоторые не подписаны | ⚠️ Проблема с подтверждением |
| Сигнатуры не совпадают | ❌ Проблема с RFC6979 |
| Сигнатуры детерминированы | ✅ RFC6979 работает |

---

## 🔧 Troubleshooting

### Проблема: Не могу захватить RNG

**Решение:**
```bash
# Проверить порт
ls -la /dev/ttyACM0

# Проверить прошивку
# Убедиться, что USE_RNG_DUMP=1
arm-none-eabi-nm build/firmware.elf | grep USE_RNG_DUMP

# Проверить бод
# По умолчанию 115200

# Если не работает — использовать screen для отладки
screen /dev/ttyACM0 115200
# Ctrl-A, Ctrl-\
```

### Проблема: Dieharder не установлен

```bash
# Установка
sudo apt install dieharder

# Проверка
dieharder -l
```

### Проблема: Не могу подписать транзакцию

**Решение:**
```bash
# Проверить IP
ping 192.168.0.10

# Проверить статус
curl http://192.168.0.10/status

# Проверить конфигурацию
# Убедиться, что USE_LWIP=1

# Проверить кнопку
# Нужно нажать кнопку на плате для подтверждения!
```

### Проблема: Низкая энтропия

**Возможные причины:**
1. TRNG отключен или неисправен
2. LCG параметры не оптимальны
3. RNG не инициализирован

**Решение:**
```bash
# Проверить инициализацию
grep -n "crypto_rng_init" Core/Src/main.c

# Проверить TRNG в Makefile
grep -n "HAL_RNG" Makefile

# Перестроить с явным USE_CRYPTO_SIGN=1
make clean
make USE_CRYPTO_SIGN=1 -j4
make flash
```

### Проблема: DIEHARDER FAIL

**Если < 5% FAIL:**
- ⚠️ Переучить с 256 MiB вместо 128 MiB
- Повторить тест 2-3 раза

**Если > 20% FAIL:**
- ❌ Серьезная проблема с RNG
- Проверить TRNG инициализацию
- Проверить LCG параметры в crypto_wallet.c

---

## 📝 Пример полного цикла тестирования

```bash
# ============================================================
# ДЕНЬ 1: RNG CAPTURE
# ============================================================

cd /data/projects/CryptoWallet

# Сборка
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash

# Запуск захвата (30 мин)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Проверка файла
ls -lh rng.bin
# -rw-r--r-- 1 user user 128M rng.bin

# ============================================================
# ДЕНЬ 2: DIEHARDER TESTS
# ============================================================

# Запуск всех тестов (3-4 часа)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder \
  | tee dieharder_results.txt

# Анализ результатов
grep -c "PASS" dieharder_results.txt
grep -c "FAIL" dieharder_results.txt
grep -c "WEAK" dieharder_results.txt

# ============================================================
# ДЕНЬ 3: SIGNING TESTS
# ============================================================

# Пересборка для подписания
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4
make flash

# Запуск тестов подписания
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# ============================================================
# ИТОГОВЫЙ ОТЧЁТ
# ============================================================

# Собрать результаты
cat > test_report.md << EOF
# CryptoWallet Security Test Report

## Date: $(date)

## RNG Analysis
- File: rng.bin (128 MiB)
- Entropy: [результаты]
- Chi-square: [результаты]

## DIEHARDER Results
- PASS: [количество]
- FAIL: [количество]
- WEAK: [количество]

## Signing Tests
- All TX signed: [да/нет]
- Deterministic: [да/нет]

EOF
```

---

## 📚 Дополнительные ресурсы

- **Документация:** `/data/projects/CryptoWallet/docs_src/crypto/`
- **RNG подробно:** `docs_src/crypto/rng-entropy.md`
- **Подписание:** `docs_src/crypto/trezor-crypto-integration.md`
- **Dieharder:** http://www.phy.duke.edu/~rgb/General/dieharder.php

---

## 📞 Помощь

```bash
# Справка по скрипту
python3 scripts/test_rng_signing_comprehensive.py --help

# Примеры
python3 scripts/test_rng_signing_comprehensive.py --help | grep "Examples:" -A 10
```

---

**Статус:** ✅ Готов к использованию  
**Версия:** 1.0  
**Дата:** 2026-03-19
