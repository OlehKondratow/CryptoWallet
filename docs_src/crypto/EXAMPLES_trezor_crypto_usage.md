# Практические примеры: trezor-crypto и Dieharder в CryptoWallet

## 1. Пример 1: Сборка и запуск для Dieharder

### 1.1 Подготовка окружения

```bash
# Перейти в корень проекта
cd /data/projects/CryptoWallet

# Создать Python venv для скриптов
python3 -m venv .venv-scripts
.venv-scripts/bin/pip install -U pip
.venv-scripts/bin/pip install -r scripts/requirements.txt

# Установить Dieharder (один раз)
sudo apt install dieharder
dieharder -l  # Проверка установки
```

### 1.2 Сборка прошивки

```bash
# Полная сборка для Dieharder
# USE_RNG_DUMP=1     → выводит сырые байты RNG на UART
# USE_CRYPTO_SIGN=1  → все криптографические модули trezor
# USE_TEST_SEED=1    → hardcoded seed для репродуцируемости

make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4

# Проверка размера
arm-none-eabi-size build/firmware.elf
#    text    data     bss     dec     hex filename
#  156000    2500   12000  170500   29d3c firmware.elf
# Text: ~156 KB (включая trezor-crypto)

# Прошивка на девайс
make flash
# или
st-flash write build/firmware.bin 0x08000000
```

### 1.3 Захват RNG потока

```bash
# Терминал 1: Наблюдение за устройством
screen /dev/ttyACM0 115200
# (ничего не должно выводиться, только сырые байты)
# Выход: Ctrl-A, Ctrl-\

# Терминал 2: Захват в файл
python3 scripts/capture_rng_uart.py \
    --port /dev/ttyACM0 \
    --out rng.bin \
    --bytes 134217728 \
    --baud 115200

# Прогресс:
#   8 MiB
#  16 MiB
#  ...
# 128 MiB
# Done: 134217728 bytes written.

# Проверка файла
ls -lh rng.bin
file rng.bin  # data (no recognized file format)
```

### 1.4 Запуск всех Dieharder тестов

```bash
# Запуск всех тестов (может занять несколько часов)
python3 scripts/run_dieharder.py --file rng.bin

# Или просто показать список доступных тестов
python3 scripts/run_dieharder.py --file rng.bin --list-tests

# Запустить конкретный тест (быстрее)
python3 scripts/run_dieharder.py --file rng.bin --test 1 --subtest 0

# Сохранить результаты
python3 scripts/run_dieharder.py --file rng.bin | tee dieharder_results.txt
```

---

## 2. Пример 2: Прямое использование trezor-crypto API в коде

### 2.1 SHA-256 хеширование

```c
// File: Core/Src/task_sign.c (пример)

#include "crypto_wallet.h"
#include <string.h>

void sign_transaction_example(void)
{
    // Данные для хеширования
    char tx_string[256];
    snprintf(tx_string, sizeof(tx_string), 
             "%s|%s|%s", 
             "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA",  // recipient
             "0.5",                                   // amount
             "BTC");                                  // currency
    
    // Буфер для хеша
    uint8_t digest[32];  // SHA-256 = 32 байта
    
    // Вызов crypto_wallet API (который использует trezor-crypto)
    int ret = crypto_hash_sha256(
        (const uint8_t *)tx_string, 
        strlen(tx_string),
        digest
    );
    
    if (ret != 0) {
        printf("SHA256 failed\n");
        return;
    }
    
    // digest теперь содержит 32-байтное хеш-значение
    printf("Digest: ");
    for (int i = 0; i < 32; i++) {
        printf("%02X", digest[i]);
    }
    printf("\n");
    
    // digest можно использовать для подписания
    // Очистка (рекомендуется для безопасности)
    // memzero(digest, sizeof(digest));
}
```

**Внутри `crypto_wallet.c`:**

```c
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[CRYPTO_SHA256_DIGEST_LEN])
{
    if (data == NULL || digest_out == NULL) return -1;
    
#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
    // Использует trezor-crypto/sha2.c
    sha256_Raw(data, len, digest_out);  // ← TREZOR!
#else
    // Использует локальный sha256_minimal.c
    sha256_minimal(data, len, digest_out);
#endif
    return 0;
}
```

---

### 2.2 BIP-32 отведение ключей

```c
// File: Core/Src/task_sign.c (пример)

#include "crypto_wallet.h"
#include "memzero.h"
#include <string.h>

void derive_bitcoin_key_example(void)
{
    // Seed (обычно получается из BIP-39 мнемоники)
    uint8_t seed[64] = {
        0xAB, 0xCD, 0xEF, ..., 0x12  // 64 байта
    };
    
    // Буфер для приватного ключа
    uint8_t priv_key[32];
    
    // Вызов crypto_wallet API (использует trezor-crypto BIP-32)
    int ret = crypto_derive_btc_m44_0_0_0_0(
        seed,         // 64-byte BIP-39 seed
        64,           // seed length
        priv_key      // output: 32-byte private scalar
    );
    
    if (ret != 0) {
        printf("Key derivation failed\n");
        memzero(seed, sizeof(seed));
        return;
    }
    
    // priv_key теперь содержит приватный ключ Bitcoin
    // (путь: m/44'/0'/0'/0/0)
    printf("Private key (hex): ");
    for (int i = 0; i < 32; i++) {
        printf("%02X", priv_key[i]);
    }
    printf("\n");
    
    // ОЧЕНЬ ВАЖНО: Очистить чувствительные данные
    memzero(seed, sizeof(seed));      // seed должен быть стёран
    // priv_key очищается внутри crypto_wallet.c,
    // но лучше очистить здесь тоже после использования
    memzero(priv_key, sizeof(priv_key));
}
```

**Внутри `crypto_wallet.c`:**

```c
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32])
{
    if (seed == NULL || priv_key_out == NULL) return -1;
    
    HDNode node;  // trezor-crypto структура
    memzero(&node, sizeof(node));
    
    // Инициализация от seed (использует trezor bip32.c)
    if (hdnode_from_seed(seed, (int)seed_len, "secp256k1", &node) != 0)
        return -1;
    
    // Отведение: m/44' (hardened)
    if (hdnode_private_ckd_prime(&node, 44) != 0) return -1;
    // Отведение: m/44'/0' (hardened)
    if (hdnode_private_ckd_prime(&node, 0) != 0) return -1;
    // Отведение: m/44'/0'/0' (hardened)
    if (hdnode_private_ckd_prime(&node, 0) != 0) return -1;
    // Отведение: m/44'/0'/0'/0 (non-hardened)
    if (hdnode_private_ckd(&node, 0) != 0) return -1;
    // Отведение: m/44'/0'/0'/0/0 (non-hardened)
    if (hdnode_private_ckd(&node, 0) != 0) return -1;
    
    // Копирование приватного ключа
    memcpy(priv_key_out, node.private_key, 32);
    
    // КРИТИЧЕСКИ: очистить HDNode (содержит приватный ключ!)
    memzero(&node, sizeof(node));
    
    return 0;
}
```

---

### 2.3 ECDSA подписание

```c
// File: Core/Src/task_sign.c (пример)

#include "crypto_wallet.h"
#include "memzero.h"
#include <string.h>

void sign_hash_example(void)
{
    // Приватный ключ (обычно из BIP-32)
    uint8_t priv_key[32] = {
        0x01, 0x02, 0x03, ..., 0x20  // 32 байта
    };
    
    // SHA-256 хеш сообщения
    uint8_t hash[32] = {
        0xAA, 0xBB, 0xCC, ..., 0xDD  // 32 байта
    };
    
    // Буфер для подписи
    uint8_t signature[64];  // 64 байта (r || s)
    
    // Вызов crypto_wallet API (использует trezor-crypto ECDSA)
    int ret = crypto_sign_btc_hash(
        priv_key,     // 32-byte private key (will be zeroed)
        hash,         // 32-byte SHA-256 digest
        signature     // output: 64-byte signature (r || s)
    );
    
    if (ret != 0) {
        printf("Signing failed\n");
        return;
    }
    
    // signature теперь содержит валидную ECDSA подпись
    printf("Signature (hex): ");
    for (int i = 0; i < 64; i++) {
        printf("%02X", signature[i]);
    }
    printf("\n");
    
    // Примечание: priv_key автоматически очищен внутри
    // crypto_sign_btc_hash() (см. memzero вызов)
}
```

**Внутри `crypto_wallet.c`:**

```c
int crypto_sign_btc_hash(uint8_t priv_key[32],
                         const uint8_t hash[CRYPTO_SHA256_DIGEST_LEN],
                         uint8_t sig_out[CRYPTO_ECDSA_SIG_LEN])
{
    if (priv_key == NULL || hash == NULL || sig_out == NULL) 
        return -1;
    
    uint8_t pby = 0;  // recovery flag (not used in compact format)
    
    // is_canonical callback (компактный формат принимает все)
    // RFC6979 использует детерминированное k (автоматически)
    int ret = ecdsa_sign_digest(
        &secp256k1,           // Кривая Bitcoin
        priv_key,             // 32-byte private key
        hash,                 // 32-byte message digest
        sig_out,              // output: 64 bytes
        &pby,                 // recovery flag
        is_canonical          // callback
    );
    
    // КРИТИЧЕСКИ: Очистить приватный ключ СРАЗУ после подписания
    memzero(priv_key, 32);
    
    return (ret == 0) ? 0 : -1;
}

// Вспомогательная функция
static int is_canonical(uint8_t by, uint8_t sig[64])
{
    (void)by;
    (void)sig;
    return 1;  // Компактный формат: принять всё
}
```

---

## 3. Пример 3: Валидация Bitcoin адреса

```c
// File: Core/Src/tx_request_validate.c (пример)

#include "tx_request_validate.h"
#include "wallet_shared.h"

#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
#include "base58.h"  // ← trezor-crypto
#endif

/**
 * @brief Validate Bitcoin P2PKH address using trezor-crypto Base58Check.
 * @param addr Bitcoin address string (e.g., "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA")
 * @return 0 if valid, -1 if invalid
 */
static int validate_bitcoin_address(const char *addr)
{
#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
    if (addr == NULL) return -1;
    
    uint8_t decoded[25];  // P2PKH address: 25 bytes
    
    // trezor-crypto base58_decode_check validates checksum
    // Returns decoded length on success, -1 on error
    int len = base58_decode_check(
        addr,                          // Input address string
        BITCOIN_ADDRESS_TYPE,          // Version byte (0x00 for P2PKH)
        decoded,                       // Output buffer
        25                             // Buffer size
    );
    
    // P2PKH addresses must be exactly 25 bytes
    if (len != 25) {
        return -1;  // Invalid
    }
    
    return 0;  // Valid!
#else
    // Fallback when USE_CRYPTO_SIGN=0
    (void)addr;
    return -1;  // No validation available
#endif
}

/**
 * @brief Full transaction request validation.
 * @param tx_json Parsed JSON transaction
 * @return 0 if valid, -1 if invalid
 */
int tx_request_validate(const wallet_tx_t *tx)
{
    if (tx == NULL) return -1;
    
    // 1. Validate recipient address (uses trezor base58)
    if (validate_bitcoin_address(tx->recipient) != 0) {
        printf("Invalid Bitcoin address: %s\n", tx->recipient);
        return -1;
    }
    
    // 2. Validate amount (regex)
    // Expected: "0.5", "1.0", "0.00001", etc.
    if (validate_amount_format(tx->amount) != 0) {
        printf("Invalid amount format: %s\n", tx->amount);
        return -1;
    }
    
    // 3. Validate currency (whitelist)
    if (validate_currency(tx->currency) != 0) {
        printf("Invalid currency: %s\n", tx->currency);
        return -1;
    }
    
    printf("Transaction validated successfully\n");
    return 0;
}
```

---

## 4. Пример 4: Полный цикл подписания

```c
// File: Core/Src/task_sign.c (реальный пример)

void sign_transaction_full_example(wallet_tx_t *tx)
{
    // 1. Валидация
    if (tx_request_validate(tx) != 0) {
        printf("Transaction validation failed\n");
        return;
    }
    
    // 2. Построение строки для хеширования
    char hash_input[256];
    snprintf(hash_input, sizeof(hash_input), "%s|%s|%s",
             tx->recipient, tx->amount,
             (tx->currency[0] != '\0') ? tx->currency : "BTC");
    
    // 3. SHA-256
    uint8_t digest[32];
    if (crypto_hash_sha256((const uint8_t *)hash_input, 
                           strlen(hash_input), 
                           digest) != 0) {
        printf("SHA256 failed\n");
        return;
    }
    
    // 4. Отображение на OLED (ожидание подтверждения)
    Task_Display_Log("CONFIRM SIGNATURE?");
    Task_Display_Log(tx->recipient);
    Task_Display_Log(tx->amount);
    
    // 5. Ожидание пользователя (30 сек тайм-аут)
    EventBits_t bits = xEventGroupWaitBits(
        g_user_event_group,
        EVENT_USER_CONFIRMED | EVENT_USER_REJECTED,
        pdTRUE, pdFALSE,
        pdMS_TO_TICKS(30000)
    );
    
    if (!(bits & EVENT_USER_CONFIRMED)) {
        printf("User rejected or timeout\n");
        memzero(digest, sizeof(digest));
        return;
    }
    
    // 6. Получение seed (из защищённого хранилища)
    uint8_t seed[64];
    if (get_wallet_seed(seed, sizeof(seed)) != 0) {
        printf("No wallet seed available\n");
        memzero(digest, sizeof(digest));
        return;
    }
    
    // 7. BIP-32 отведение ключа
    uint8_t priv_key[32];
    if (crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key) != 0) {
        printf("Key derivation failed\n");
        memzero(seed, sizeof(seed));
        memzero(digest, sizeof(digest));
        return;
    }
    
    // 8. ECDSA подписание
    uint8_t signature[64];
    if (crypto_sign_btc_hash(priv_key, digest, signature) != 0) {
        printf("Signing failed\n");
        memzero(seed, sizeof(seed));
        memzero(digest, sizeof(digest));
        memzero(signature, sizeof(signature));
        return;
    }
    
    // 9. Сохранение подписи
    memcpy(g_last_sig, signature, 64);
    
    // 10. Очистка (очень важно!)
    memzero(seed, sizeof(seed));
    memzero(digest, sizeof(digest));
    // priv_key уже очищен в crypto_sign_btc_hash
    // signature уже очищен? Нет, давайте очистим:
    memzero(signature, sizeof(signature));
    
    printf("Signature complete\n");
    
    // 11. Отправка ответа
    // HTTP: GET /tx/signed → JSON с signature
    // WebUSB: EP2 → 64-byte binary signature
}
```

---

## 5. Пример 5: Конфигурация Makefile

```makefile
# File: Makefile (выдержка)

# Флаги для Dieharder тестирования
USE_CRYPTO_SIGN ?= 1          # 1 = full trezor-crypto, 0 = minimal
USE_TEST_SEED ?= 0            # 1 = hardcoded seed (dev only!)
USE_RNG_DUMP ?= 0             # 1 = raw RNG output on UART
USE_LWIP ?= 1                 # 1 = Ethernet
USE_WEBUSB ?= 1               # 1 = USB WebUSB

# Директория trezor-crypto
TREZOR_CRYPTO_DIR = ThirdParty/trezor-crypto

# Если USE_CRYPTO_SIGN=1, линкуются модули
ifeq ($(USE_CRYPTO_SIGN),1)
  CRYPTO_OBJS = \
    $(TREZOR_CRYPTO_DIR)/bip39.o \
    $(TREZOR_CRYPTO_DIR)/bip32.o \
    $(TREZOR_CRYPTO_DIR)/ecdsa.o \
    $(TREZOR_CRYPTO_DIR)/secp256k1.o \
    $(TREZOR_CRYPTO_DIR)/sha2.o \
    $(TREZOR_CRYPTO_DIR)/hmac.o \
    $(TREZOR_CRYPTO_DIR)/pbkdf2.o \
    $(TREZOR_CRYPTO_DIR)/base58.o \
    $(TREZOR_CRYPTO_DIR)/rfc6979.o \
    # ... ещё несколько модулей
  
  DEFINES += -DUSE_CRYPTO_SIGN=1
else
  CRYPTO_OBJS = Core/Src/sha256_minimal.o
  DEFINES += -DUSE_CRYPTO_SIGN=0
endif

# Компиляция
C_SOURCES = \
  Core/Src/crypto_wallet.c \
  Core/Src/task_sign.c \
  Core/Src/tx_request_validate.c \
  # ...

# Компилятор ARM
CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += $(DEFINES)
CFLAGS += -I$(TREZOR_CRYPTO_DIR)  # Пути заголовков trezor

# Линкинг
LDFLAGS = -Wl,--gc-sections
LDFLAGS += -T$(LINKER_SCRIPT)

firmware.elf: $(C_OBJS) $(CRYPTO_OBJS)
    $(CC) $(LDFLAGS) -o $@ $^
```

---

## 6. Пример 6: Проверка интеграции

```bash
#!/bin/bash
# File: scripts/check_integration.sh

set -e

echo "=== Checking trezor-crypto integration ==="

# 1. Проверить наличие символов в ELF
echo "1. Checking symbols..."
arm-none-eabi-nm build/firmware.elf | grep -q "bip39_from_data" && \
    echo "✓ bip39 symbols found" || echo "✗ bip39 symbols missing"

arm-none-eabi-nm build/firmware.elf | grep -q "hdnode_from_seed" && \
    echo "✓ bip32 symbols found" || echo "✗ bip32 symbols missing"

arm-none-eabi-nm build/firmware.elf | grep -q "ecdsa_sign_digest" && \
    echo "✓ ecdsa symbols found" || echo "✗ ecdsa symbols missing"

# 2. Размер бинарника
echo ""
echo "2. Binary size:"
arm-none-eabi-size build/firmware.elf | tail -1

# 3. Проверить random_buffer
echo ""
echo "3. Checking RNG hook..."
arm-none-eabi-nm build/firmware.elf | grep "random_buffer" && \
    echo "✓ random_buffer found"

# 4. Проверить memzero
echo ""
echo "4. Checking memzero (security)..."
arm-none-eabi-nm build/firmware.elf | grep -q "memzero" && \
    echo "✓ memzero found"

echo ""
echo "=== Integration check complete ==="
```

---

**Конец примеров**
