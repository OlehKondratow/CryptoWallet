# Анализ trezor-firmware

Разбор структуры и паттернов `/data/projects/trezor-firmware` для интеграции в CryptoWallet.

## 1. Структура репозитория

```
trezor-firmware/
├── crypto/           # Криптобиблиотека (standalone, C)
├── core/             # Trezor T firmware (Rust + C)
├── legacy/           # Trezor One firmware (C, libopencm3)
├── storage/          # NORCOW storage
├── common/           # Coin definitions, protobuf
└── vendor/           # Submodules
```

**Crypto** — самодостаточная, используется и в core, и в legacy.

---

## 2. RNG (random_buffer)

### Legacy (Trezor One, STM32F2)

```
legacy/rng.c
```

- Прямой доступ к регистрам RNG: `RNG_SR`, `RNG_DR`
- `rng_get_u32()` — проверка FIPS (нет двух подряд одинаковых значений)
- `random_buffer()` — заполняет буфер через `rng_get_u32()`
- `#if !EMULATOR` — на эмуляторе не используется

### Core (Trezor T, STM32)

```
core/embed/sys/rng/stm32/rng.c
```

- `rng_init()` — `__HAL_RCC_RNG_CLK_ENABLE()`, `RNG->CR = RNG_CR_RNGEN`
- `rng_read_u32()` — ожидание `RNG_SR_DRDY`, проверка FIPS
- `rng_fill_buffer()` — заполнение буфера
- `random_buffer()` — обёртка над `rng_fill_buffer()`

### crypto/rand.h

```c
void random_buffer(uint8_t *buf, size_t len);  // Должна быть реализована платформой

static inline uint32_t random32(void) {
  uint32_t r = 0;
  random_buffer((uint8_t *)&r, sizeof(r));
  return r;
}
```

**Важно:** `random_buffer` — единственная платформо-зависимая функция в crypto. Реализация в `legacy/rng.c` или `core/embed/sys/rng/stm32/rng.c`.

---

## 3. BIP-39

### Файлы

- `crypto/bip39.c`, `crypto/bip39.h`
- `crypto/bip39_english.c` — словарь 2048 слов

### API

```c
const char *mnemonic_from_data(const uint8_t *data, int len);  // len: 16, 20, 24, 28, 32
void mnemonic_clear(void);
int mnemonic_check(const char *mnemonic);
void mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                      uint8_t seed[64],
                      void (*progress_callback)(uint32_t, uint32_t));
```

### Особенности

- `mnemonic_from_data` использует `sha256_Raw` для checksum
- `mnemo` — статический буфер с `CONFIDENTIAL`
- `mnemonic_clear()` — обнуление после использования
- `USE_BIP39_CACHE` — кэш seed для ускорения

### Отличия от trezor-crypto

- `mnemonic_to_bits` вместо `mnemonic_to_entropy`
- `BIP39_WORDLIST_ENGLISH` — внешний массив

---

## 4. BIP-32 (HDNode)

### Файлы

- `crypto/bip32.c`, `crypto/bip32.h`
- `crypto/ecdsa.h`, `crypto/secp256k1.c`

### Структура HDNode

```c
typedef struct {
  uint32_t depth;
  uint32_t child_num;
  uint8_t chain_code[32];
  uint8_t private_key[32];
  uint8_t private_key_extension[32];
  uint8_t public_key[33];
  bool is_public_key_set;      // Новое по сравнению с trezor-crypto
  const curve_info *curve;
} HDNode;
```

### API

```c
int hdnode_from_seed(const uint8_t *seed, int seed_len, const char *curve, HDNode *out);
int hdnode_private_ckd(HDNode *inout, uint32_t i);
#define hdnode_private_ckd_prime(X, I) hdnode_private_ckd((X), ((I) | 0x80000000))
int hdnode_sign_digest(HDNode *node, const uint8_t *digest, uint8_t *sig, uint8_t *pby, ...);
```

### Путь m/44'/0'/0'/0/0

```c
hdnode_from_seed(seed, 64, "secp256k1", &node);
hdnode_private_ckd_prime(&node, 44);   // 44'
hdnode_private_ckd_prime(&node, 0);  // 0'
hdnode_private_ckd_prime(&node, 0);  // 0'
hdnode_private_ckd(&node, 0);        // 0
hdnode_private_ckd(&node, 0);        // 0
```

---

## 5. ECDSA (secp256k1)

### Файлы

- `crypto/ecdsa.c`, `crypto/ecdsa.h`
- `crypto/secp256k1.c`, `crypto/secp256k1.h`

### Подпись digest

```c
int ecdsa_sign_digest(const ecdsa_curve *curve, const uint8_t *priv_key,
                      const uint8_t *digest, uint8_t *sig, uint8_t *pby,
                      int (*is_canonical)(uint8_t by, uint8_t sig[64]));
```

- `curve` — `&secp256k1`
- `digest` — 32 байта (SHA-256)
- `sig` — 64 байта (r || s, compact)
- `pby` — recovery id (для Ethereum)

---

## 6. Безопасность

### CONFIDENTIAL

```c
// options.h
#define CONFIDENTIAL   // Пустой или __attribute__((section(".confidential")))
```

Используется для приватных ключей, seed, мнемоник — помечает данные для особой секции или анализа.

### memzero

```c
// crypto/memzero.h
void memzero(void* const pnt, const size_t len);
```

Вызывается после каждого использования конфиденциальных данных.

### Примеры в signing.c

```c
static CONFIDENTIAL HDNode root;
static CONFIDENTIAL HDNode node;
static uint8_t CONFIDENTIAL privkey[32];
static uint8_t sig[64];
// ...
memzero(privkey, 32);
memzero(&node, sizeof(node));
```

---

## 7. Сборка (legacy)

### Makefile

- `legacy/rng.c` — входит в `libtrezor.a`
- `crypto/` — отдельная сборка
- `vendor/trezor-storage` — submodule

### Зависимости

- libopencm3 (STM32F2 для Trezor One)
- Python для protobuf

---

## 8. Рекомендации для CryptoWallet

| Компонент | trezor-firmware | CryptoWallet |
|-----------|-----------------|--------------|
| **random_buffer** | legacy/rng.c или core/embed | crypto_wallet.c (override) |
| **RNG** | Прямой доступ к RNG | HAL_RNG или прямой доступ |
| **BIP-39** | crypto/bip39.c | Подключить crypto/ |
| **BIP-32** | crypto/bip32.c | Подключить crypto/ |
| **ECDSA** | crypto/ecdsa.c | Подключить crypto/ |
| **memzero** | crypto/memzero.c | Уже есть в Core/Src/memzero.c |

### Варианты интеграции

1. **Копировать crypto/** — скопировать `crypto/` из trezor-firmware, реализовать `random_buffer` в проекте.
2. **Submodule** — `git submodule add` trezor-firmware, собирать только `crypto/`.
3. **trezor-crypto** — старый репозиторий, API совместим, но архивный.

### Минимальный набор crypto для Bitcoin

- bip39.c, bip39_english.c, bip39.h
- bip32.c, bip32.h
- ecdsa.c, ecdsa.h
- secp256k1.c, secp256k1.h, secp256k1.table
- sha2.c, sha2.h
- hmac.c, hmac.h
- pbkdf2.c, pbkdf2.h
- bignum.c, bignum.h
- memzero.c, memzero.h
- rand.c, rand.h (или только реализация random_buffer)
- hasher.c, hasher.h
- base58.c, base58.h
- ripemd160.c, ripemd160.h
- curves.c, curves.h
- options.h
