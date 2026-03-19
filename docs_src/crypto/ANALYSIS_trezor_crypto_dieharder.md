# Анализ trezor-crypto и Dieharder в CryptoWallet

**Дата:** 2026-03-19  
**Цель:** Полный анализ интеграции криптографической библиотеки trezor-crypto и использования Dieharder для тестирования RNG.

---

## 1. Архитектура интеграции trezor-crypto

### 1.1 Расположение и структура

```
CryptoWallet/
├── ThirdParty/trezor-crypto/          # Внешняя библиотека (MIT License)
│   ├── bip39.c/h                      # BIP-39 мнемоника
│   ├── bip32.c/h                      # Иерархическое отведение ключей
│   ├── secp256k1.c/h                  # Эллиптическая кривая Bitcoin
│   ├── ecdsa.c/h                      # ECDSA подписание
│   ├── rfc6979.c/h                    # Детерминированное k
│   ├── sha2.c/h                       # SHA-256/SHA-512
│   ├── rand.c/h                       # RNG интерфейс
│   ├── hmac.c/h, pbkdf2.c/h          # PBKDF2 (BIP-39 → Seed)
│   ├── base58.c/h                     # Base58Check (Bitcoin адреса)
│   └── (ещё 70+ файлов)               # ED25519, Blake, Groestl, AES...
│
├── Core/Src/crypto_wallet.c           # АДАПТЕР (glue layer)
├── Core/Inc/crypto_wallet.h           # Публичный интерфейс
├── Core/Src/task_sign.c               # Основная задача подписания
└── Core/Src/sha256_minimal.c          # Фолбэк (USE_CRYPTO_SIGN=0)
```

### 1.2 Слои интеграции

```
┌─────────────────────────────────────────────┐
│   Приложение (task_sign, task_net)         │
├─────────────────────────────────────────────┤
│   crypto_wallet.h (публичный API)          │
│   - crypto_rng_init()                       │
│   - crypto_entropy_to_mnemonic_12()         │
│   - crypto_derive_btc_m44_0_0_0_0()         │
│   - crypto_sign_btc_hash()                  │
│   - crypto_hash_sha256()                    │
├─────────────────────────────────────────────┤
│   crypto_wallet.c (реализация)              │
│   - random_buffer()          [RNG hook]     │
│   - random_reseed(), random32()             │
│   - TRNG + LCG entropy mixing               │
├─────────────────────────────────────────────┤
│   trezor-crypto (внешняя библиотека)       │
│   - bip39, bip32, ecdsa, secp256k1, sha2   │
├─────────────────────────────────────────────┤
│   STM32 HAL                                  │
│   - HAL_RNG_GenerateRandomNumber()          │
│   - HAL_GetTick()                           │
└─────────────────────────────────────────────┘
```

---

## 2. Использование trezor-crypto в проекте

### 2.1 Криптографические функции

| Модуль | Функция | Назначение | Используется |
|--------|---------|-----------|-------------|
| **bip39** | `mnemonic_from_data()` | 128 бит → 12 слов | `crypto_entropy_to_mnemonic_12()` |
| **bip32** | `hdnode_from_seed()` | Инициализация узла | `crypto_derive_btc_m44_0_0_0_0()` |
| **bip32** | `hdnode_private_ckd_prime()` | Жёсткое отведение | m/44', m/44'/0', m/44'/0'/0' |
| **bip32** | `hdnode_private_ckd()` | Мягкое отведение | m/44'/0'/0'/0, m/44'/0'/0'/0/0 |
| **ecdsa** | `ecdsa_sign_digest()` | ECDSA подпись | `crypto_sign_btc_hash()` |
| **secp256k1** | `secp256k1` (curve) | Параметры кривой | `ecdsa_sign_digest()` |
| **rfc6979** | (внутри ecdsa) | Детерминированное k | Автоматический вызов |
| **sha2** | `sha256_Raw()` | SHA-256 | `crypto_hash_sha256()` |
| **base58** | `base58_decode_check()` | Валидация адреса | `tx_request_validate.c` |
| **rand** | (не используется) | RNG интерфейс | Переопределён в `random_buffer()` |

### 2.2 Путь вызовов: HTTP POST → Подпись

```
1. HTTP POST /tx (JSON)
   ↓
   [task_net.c] → xQueueSend(g_tx_queue, ...)
   ↓
2. [task_sign.c] → xQueueReceive(g_tx_queue, ...)
   ↓
3. tx_request_validate() → tx_request_validate.c
   • base58_decode_check(recipient_addr)     [trezor-crypto]
   • regex_match(amount, "^[0-9.]+$")
   • whitelist_check(currency)
   ↓
4. build_hash_input() → "recipient|amount|currency"
   ↓
5. crypto_hash_sha256(data, ...) → 32 байта
   • USE_CRYPTO_SIGN=1 → sha256_Raw()        [trezor-crypto]
   • USE_CRYPTO_SIGN=0 → sha256_minimal()    [локальный]
   ↓
6. xEventGroupWaitBits(g_user_event_group, ...)
   • Ожидание подтверждения пользователя (кнопка, тайм-аут 30 сек)
   ↓
7. get_wallet_seed() → 64 байта
   • USE_TEST_SEED=1 → hardcoded test vector
   • USE_TEST_SEED=0 → weak stub (-1) или secure storage
   ↓
8. crypto_derive_btc_m44_0_0_0_0()
   • hdnode_from_seed()              [trezor-crypto]
   • hdnode_private_ckd_prime(44, 0, 0)
   • hdnode_private_ckd(0, 0)
   • Output: 32-byte private key
   ↓
9. crypto_sign_btc_hash(priv_key, hash, ...)
   • ecdsa_sign_digest(&secp256k1, ...)     [trezor-crypto]
   • Output: 64-byte signature (r||s)
   • memzero(priv_key)               [очистка!]
   ↓
10. task_display / HTTP response
    • g_last_sig = signature
    • WebUSB или HTTP JSON ответ
```

### 2.3 Точки интеграции в Core/Src/

```c
// Core/Src/crypto_wallet.c (АДАПТЕР)
───────────────────────────────────────────────

#if USE_CRYPTO_SIGN == 1
#include "bip39.h"        ← trezor-crypto
#include "bip32.h"        ← trezor-crypto
#include "ecdsa.h"        ← trezor-crypto
#include "secp256k1.h"    ← trezor-crypto
#include "sha2.h"         ← trezor-crypto
#include "rand.h"         ← trezor-crypto
#include "rfc6979.h"      ← (используется внутри ecdsa)
#endif

// RNG hookpoint для trezor-crypto
void random_buffer(uint8_t *buf, size_t len)
{
    // STM32 TRNG XOR entropy pool
    HAL_RNG_GenerateRandomNumber(&hrng, &rng_val)
    rng_val ^= s_rng_entropy_pool  // LCG mixing
}

// BIP-39
int crypto_entropy_to_mnemonic_12(...)
{
    mnemonic_from_data(entropy, 16)    ← trezor bip39.c
}

// BIP-32
int crypto_derive_btc_m44_0_0_0_0(...)
{
    hdnode_from_seed(seed, 64, "secp256k1", &node)
    hdnode_private_ckd_prime(&node, 44)   // m/44'
    hdnode_private_ckd_prime(&node, 0)    // m/44'/0'
    hdnode_private_ckd_prime(&node, 0)    // m/44'/0'/0'
    hdnode_private_ckd(&node, 0)          // m/44'/0'/0'/0
    hdnode_private_ckd(&node, 0)          // m/44'/0'/0'/0/0
    memcpy(priv_key_out, node.private_key, 32)
    memzero(&node, sizeof(node))
}

// ECDSA подписание
int crypto_sign_btc_hash(...)
{
    ecdsa_sign_digest(&secp256k1, priv_key, hash, sig_out, &pby, ...)
    memzero(priv_key, 32)  // RFC6979 используется автоматически
}

// SHA-256
int crypto_hash_sha256(...)
{
    #if USE_CRYPTO_SIGN
    sha256_Raw(data, len, digest_out)  ← trezor sha2.c
    #else
    sha256_minimal(data, len, digest_out)
    #endif
}
```

### 2.4 RNG: TRNG + LCG Entropy Pool

```c
// Core/Src/crypto_wallet.c (строки 41-94)
───────────────────────────────────────────────

static volatile uint32_t s_rng_entropy_pool = 0U;

// Hook: вызывается trezor-crypto для всех случайных значений
void random_buffer(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i += 4) {
        uint32_t rng_val;
        
        // Вариант 1: TRNG есть
        if (hrng.Instance != NULL && 
            HAL_RNG_GenerateRandomNumber(&hrng, &rng_val) == HAL_OK)
        {
            // XOR с энтропийным пулом
            rng_val ^= s_rng_entropy_pool;
            
            // LCG обновление (Numerical Recipes параметры)
            s_rng_entropy_pool = (s_rng_entropy_pool * 1664525U) + 1013904223U;
        }
        // Вариант 2: TRNG недоступен
        else
        {
            rng_val = s_rng_entropy_pool;
            s_rng_entropy_pool = (s_rng_entropy_pool * 1664525U) + 1013904223U;
        }
        
        // Копирование в буфер вывода
        size_t n = (len - i > 4) ? 4 : (len - i);
        memcpy(buf + i, &rng_val, n);
    }
}

// Инициализация (вызывается один раз из main.c)
int crypto_rng_init(void)
{
    if (hrng.Instance == NULL) return -1;
    
    // Начальное заполнение пула от таймера системы
    uint32_t adc_noise = HAL_GetTick();
    for (volatile int i = 0; i < 8; i++) {
        adc_noise ^= (adc_noise << 13);
        adc_noise ^= (adc_noise >> 17);
        adc_noise ^= (adc_noise << 5);
    }
    random_reseed(adc_noise);
    return 0;
}

// API: явный reseed (внешний запуск)
void random_reseed(const uint32_t value)
{
    s_rng_entropy_pool ^= value;
}

// API: единое 32-битное случайное число
uint32_t random32(void)
{
    uint32_t r;
    random_buffer((uint8_t *)&r, sizeof(r));
    return r;
}
```

---

## 3. DIEHARDER: Тестирование качества RNG

### 3.1 Назначение DIEHARDER

**DIEHARDER** — это набор статистических тестов для генераторов случайных чисел:
- Обнаружение смещений и корреляций
- Проверка равномерности распределения
- Индентификация периодичности и структур

**⚠️ Важно:** DIEHARDER не сертифицирует криптографический RNG, но помогает выявить **грубые дефекты** (bias, patterns).

### 3.2 Рабочий процесс Dieharder в CryptoWallet

```
┌────────────────────────────────────────────┐
│ 1. Подготовка прошивки                    │
│    USE_RNG_DUMP=1 → выводит ТОЛЬКО       │
│    сырые байты на UART (без текста)       │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 2. Запуск на девайсе                       │
│    Подключение: /dev/ttyACM0, 115200 baud │
│    Прошивка запускается, генерирует RNG    │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 3. Захват потока с помощью Python         │
│    scripts/capture_rng_uart.py             │
│    --port /dev/ttyACM0                     │
│    --out rng.bin                           │
│    --bytes 134217728  (128 MiB)            │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 4. Проверка размера файла                  │
│    -rw-r--r-- 1 user user 134217728        │
│    rng.bin (exactly 128 MiB)               │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 5. Установка dieharder                     │
│    sudo apt install dieharder              │
│    dieharder -l  (проверка)                │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 6. Запуск тестов                           │
│    scripts/run_dieharder.py                │
│    --file rng.bin                          │
│    [--test 1 --subtest 0]  (опционально)   │
└────────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────────┐
│ 7. Анализ результатов                      │
│    Поиск p-value < 0.0001 или > 0.9999     │
│    (признак систематического смещения)     │
└────────────────────────────────────────────┘
```

### 3.3 Скрипты Dieharder

#### **scripts/capture_rng_uart.py**

```python
# Захват сырых байт с UART в файл

capture(
    port="/dev/ttyACM0",
    baud=115200,
    out_path="rng.bin",
    total_bytes=134_217_728,      # 128 MiB (рекомендуемый минимум)
    skip_bytes=0                   # Пропустить первые N байт
)

# Вывод: каждые 8 MiB выводит прогресс
#   8 MiB
#  16 MiB
#  ...
# 128 MiB
# Done: 134217728 bytes written.
```

#### **scripts/run_dieharder.py**

```python
# Запуск dieharder тестов на файле

run_dieharder(
    file="rng.bin",
    dieharder="/usr/bin/dieharder",
    test=None,         # None = все тесты (-a)
    subtest=None,      # Для конкретного теста
    psamples=None      # Переопределить количество выборок
)

# Команда: dieharder -g 201 -f rng.bin -a
#          201 = file_input_raw (unsigned chars from file)
#          -a = all tests
```

### 3.4 Интерпретация результатов DIEHARDER

```
====
dieharder result: PASS | WEAK | FAIL | ERROR
====

# Каждый тест выводит p-value:
Test 1: ...
  p-value = 0.523  ✓ PASS (0.001 < p < 0.999)
  p-value = 0.0001 ✗ FAIL (< 0.001, смещение)
  p-value = 0.9999 ✗ FAIL (> 0.999, антикорреляция)

# Интерпретация:
┌─────────────┬──────────────────┬───────────────┐
│  p-value    │     Результат    │   Действие    │
├─────────────┼──────────────────┼───────────────┤
│ 0.001-0.999 │ PASS             │ OK            │
│ 0.0001-0.001│ WEAK             │ Подозрение    │
│ < 0.0001    │ FAIL             │ Проблема!     │
│ > 0.9999    │ FAIL             │ Проблема!     │
└─────────────┴──────────────────┴───────────────┘

# Примеры проблем:
- Много FAIL на одном тесте → смещение в RNG
- WEAK на нескольких → переджать больший файл (>256 MiB)
- ERROR → формат файла или dieharder ошибка
```

### 3.5 Список тестов DIEHARDER

```bash
# Просмотр доступных тестов
$ dieharder -l

# Пример выходных данных:
test_num = 0   diehard_birthdays
test_num = 1   diehard_operm5
test_num = 2   diehard_rank_32x32
test_num = 3   diehard_rank_6x8
test_num = 4   diehard_bitstream
test_num = 5   diehard_operm5
test_num = 6   diehard_rank_32x32
test_num = 7   diehard_rank_6x8
test_num = 8   diehard_bitstream
test_num = 9   diehard_operm5
test_num = 10  diehard_rank_32x32
test_num = 11  diehard_rank_6x8
test_num = 12  diehard_bitstream
test_num = 13  diehard_operm5
test_num = 14  diehard_rank_32x32
test_num = 15  diehard_rank_6x8
test_num = 16  diehard_bitstream

# ... (всего ~100+ тестов)

# Запуск одного конкретного теста:
dieharder -g 201 -f rng.bin -d 1 -p 0
```

---

## 4. Использование в Core/Src/ и Core/Inc/

### 4.1 Файлы проекта, использующие trezor-crypto

| Файл | Функция | Вызваемые модули trezor |
|------|---------|----------------------|
| `crypto_wallet.c` | Основной адаптер | bip39, bip32, ecdsa, sha2, rand, rfc6979, secp256k1 |
| `task_sign.c` | FSM подписания | crypto_wallet (опосредованно) |
| `tx_request_validate.c` | Валидация адреса | base58 (decode_check) |
| `sha256_minimal.c` | Фолбэк | (независимый sha256) |

### 4.2 Заголовочные файлы (Core/Inc/)

```c
// Core/Inc/crypto_wallet.h
────────────────────────────

#define CRYPTO_ENTROPY_128_BITS    16U   // BIP-39 128 бит
#define CRYPTO_MNEMONIC_MAX_LEN    256U  // Макс мнемоника
#define CRYPTO_SHA256_DIGEST_LEN   32U   // SHA-256 выход
#define CRYPTO_ECDSA_SIG_LEN       64U   // Компактная подпись

// API функции
int crypto_rng_init(void);
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[16],
                                  char *mnemonic_out, size_t size);
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t len,
                                  uint8_t priv_key_out[32]);
int crypto_sign_btc_hash(uint8_t priv_key[32],
                        const uint8_t hash[32],
                        uint8_t sig_out[64]);
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[32]);
```

### 4.3 Точки вызовов в основных задачах

#### **task_sign.c → crypto_wallet**

```c
// 1. Инициализация (main.c)
crypto_rng_init();

// 2. Получение мнемоники (используется редко, для setup)
crypto_entropy_to_mnemonic_12(entropy, mnemonic_buf, sizeof(mnemonic_buf));

// 3. Хеширование транзакции (всегда)
crypto_hash_sha256((const uint8_t *)hash_input_str, len, digest);

// 4. Подтверждение пользователя (wait confirmation)
xEventGroupWaitBits(g_user_event_group, EVENT_USER_CONFIRMED, ...);

// 5. Получение seed из хранилища
get_wallet_seed(seed_buf, 64);

// 6. Отведение ключа
crypto_derive_btc_m44_0_0_0_0(seed_buf, 64, priv_key);

// 7. Подписание
crypto_sign_btc_hash(priv_key, digest, signature);

// 8. Отправка ответа
WebUSB_NotifySignatureReady(signature, 64);
```

#### **tx_request_validate.c → trezor base58**

```c
// Прямой вызов trezor-crypto (не через crypto_wallet)
#if USE_CRYPTO_SIGN == 1
#include "base58.h"  // trezor-crypto

int validate_bitcoin_address(const char *addr)
{
    uint8_t decoded[25];
    int len = base58_decode_check(addr, BITCOIN_ADDRESS_TYPE, decoded, 25);
    return (len == 25) ? 0 : -1;  // P2PKH (25 байт)
}
#endif
```

---

## 5. Сценарии тестирования

### 5.1 Проверка подписания с Dieharder

**Цель:** Убедиться, что RNG достаточно хорош для криптографии.

**Поток:**

1. **Сборка с USE_RNG_DUMP**
   ```bash
   make clean
   make USE_CRYPTO_SIGN=1 USE_RNG_DUMP=1 USE_TEST_SEED=1
   make flash
   ```

2. **Запуск прошивки, вывод RNG потока**
   ```bash
   # Прошивка начинает выводить сырые байты на UART
   # Никакого текстового вывода!
   ```

3. **Захват в отдельном терминале**
   ```bash
   python3 scripts/capture_rng_uart.py \
     --port /dev/ttyACM0 \
     --out rng.bin \
     --bytes 134217728
   
   # Прогресс каждые 8 MiB, ~30 мин на 115200 baud
   ```

4. **Установка Dieharder (один раз)**
   ```bash
   sudo apt install dieharder
   ```

5. **Запуск всех тестов**
   ```bash
   python3 scripts/run_dieharder.py --file rng.bin
   
   # Выход (30 минут — несколько часов)
   diehard_birthdays
   diehard_operm5
   diehard_rank_32x32
   ...
   ```

6. **Анализ результатов**
   - Ищите FAIL или WEAK (особенно < 0.0001 или > 0.9999)
   - Проверьте, не скоррелированы ли сбои с определённым тестом
   - При необходимости: пересоберите с другим начальным seed или уменьшите параметры LCG

### 5.2 Проверка подписания (функциональная)

```python
# scripts/test_plan_signing_rng.py генерирует чек-лист

# Проверочные точки:
1. Сборка: USE_CRYPTO_SIGN=1, USE_TEST_SEED=1
2. POST /tx валидный JSON → enqueue в g_tx_queue
3. Нажать кнопку → подтверждение
4. UART: "Signed OK" в логе
5. GET /tx/signed → JSON с 64-байтной подписью
6. Проверить: одинаковые входы → детерминированная подпись (RFC6979)
7. Безопасность: нет seed/privkey в ответах
```

---

## 6. Компиляционные флаги (влияние на trezor-crypto)

| Флаг | Значение | Эффект |
|------|----------|--------|
| `USE_CRYPTO_SIGN` | 1 | Линкуются все модули trezor (bip39, bip32, ecdsa, sha2, …) — ~150 KB |
| `USE_CRYPTO_SIGN` | 0 | Только sha2.c от trezor, остальное — stubs — ~5 KB |
| `USE_TEST_SEED` | 1 | Hardcoded test vector (только для разработки!) |
| `USE_RNG_DUMP` | 1 | Выводит сырые байты RNG на UART (для Dieharder) |
| `USE_LWIP` | 1 | Ethernet для HTTP /tx endpoint |
| `USE_WEBUSB` | 1 | WebUSB endpoint для подписания |

**Рекомендуемая конфигурация для Dieharder:**
```bash
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1
```

---

## 7. Безопасность и лучшие практики

### 7.1 Обработка чувствительных данных

```c
// ✓ ПРАВИЛЬНО: очистка после использования
uint8_t priv_key[32];
crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key);
crypto_sign_btc_hash(priv_key, hash, sig_out);
memzero(priv_key, 32);  // Очистка
memzero(seed, 64);      // Очистка seed

// ✗ НЕПРАВИЛЬНО: оставить в памяти
uint8_t priv_key[32];
crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key);
// ... другой код
// Ключ всё ещё в памяти!
```

### 7.2 RFC6979 (Детерминированное k)

**Преимущество:** Одинаковый hash → одинаковая подпись

```c
// Подпись детерминирована (не случайна)
Message A, Key K → Signature S₁
Message A, Key K → Signature S₁  (ИДЕНТИЧНО!)

// Vs. обычная ECDSA (случайное k)
Message A, Key K → Signature S₁
Message A, Key K → Signature S₂  (Разные!)
```

### 7.3 Смешивание энтропии TRNG + LCG

```
Плюсы:
- TRNG обеспечивает физическую случайность
- LCG добавляет временную вариативность
- XOR комбинирует оба источника

Минусы:
- LCG сам по себе криптографически неподходящ
- Требует тестирования Dieharder
- При отказе TRNG → только LCG (слабо)
```

---

## 8. Проблемные области и диагностика

### 8.1 Типичные ошибки при Dieharder

| Проблема | Причина | Решение |
|----------|---------|---------|
| FAIL на большинстве тестов | Смещение TRNG или LCG параметры | Переинициализировать seed, проверить HAL_RNG |
| Error: file too small | < 1 MiB данных | Переучить с 128 MiB (134217728 bytes) |
| Parse error | Текст смешан с двоичными данными | USE_RNG_DUMP=1, отключить printf на UART |
| Segfault | dieharder не установлен | `sudo apt install dieharder` |
| WEAK на нескольких | Граничный случай | Переучить с 256+ MiB или проверить LCG |

### 8.2 Проверка интеграции trezor-crypto

```bash
# 1. Проверить линкованные модули
arm-none-eabi-nm build/firmware.elf | grep -E "bip39|bip32|ecdsa"

# 2. Проверить размер
arm-none-eabi-size build/firmware.elf
#   text    data     bss     dec     hex filename
# 150000    1000   10000  161000   2750f firmware.elf

# 3. Проверить наличие trezor-crypto в карте памяти
arm-none-eabi-objdump -h build/firmware.elf | grep -E "\.text|\.rodata"

# 4. Выполнить smoke test
./scripts/bootloader_secure_signing_test.py --elf-audit-only --no-build --no-flash
```

---

## 9. Документация и ссылки

### 9.1 Файлы в проекте

- `Core/Src/crypto_wallet.c` — Основной адаптер
- `Core/Src/task_sign.c` — Использование
- `scripts/capture_rng_uart.py` — Захват RNG
- `scripts/run_dieharder.py` — Тестирование
- `scripts/test_plan_signing_rng.py` — Тестовый план
- `docs_src/trezor-crypto-integration.md` — Техническая документация
- `docs_src/rng-entropy.md` — Подробности RNG

### 9.2 Внешние ссылки

- **trezor-crypto:** https://github.com/trezor/trezor-crypto (MIT License)
- **BIP-32:** https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
- **BIP-39:** https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki
- **RFC6979:** https://tools.ietf.org/html/rfc6979
- **Dieharder:** http://www.phy.duke.edu/~rgb/General/dieharder.php

---

## 10. Рекомендации и улучшения

### 10.1 Краткосрочные улучшения

```
1. ✓ Запустить Dieharder на 128 MiB данных
2. ✓ Документировать результаты p-value
3. ✓ Сравнить с известными хорошими RNG
4. ⚠️ Рассмотреть увеличение LCG seed до 64 бит
```

### 10.2 Среднесрочные улучшения

```
5. Рассмотреть HMAC-DRBG вместо простого LCG
6. Добавить тестирование на других криптографических функциях
7. Обновить trezor-crypto до последней версии
8. Расширить тестовый план для всех модулей (BIP-32, ECDSA, ...)
```

### 10.3 Долгосрочные улучшения

```
9. Формальная верификация безопасности
10. Сертификация по FIPS 140-2 Level 2
11. Интеграция с Secure Element (ATECC608)
12. Поддержка других криптографических кривых (P-256, Ed25519)
```

---

**Конец анализа**

Дата создания: 2026-03-19  
Версия: 1.0
