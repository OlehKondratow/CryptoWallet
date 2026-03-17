# Интеграция trezor-crypto

Руководство по подключению trezor-crypto к CryptoWallet (STM32H743).

## Лицензия

**trezor-crypto** распространяется под **MIT License** (Copyright © 2013 Tomas Dzetkulic, Pavol Rusnak).

- Разрешено: использование, модификация, распространение, коммерческое применение, статическая линковка.
- Обязательно: сохранять копирайт и текст лицензии в дистрибутиве.
- Полный текст: `ThirdParty/trezor-crypto/LICENSE`.

Лицензионных ограничений для CryptoWallet нет — MIT не требует открывать исходники проекта.

## 1. Добавление trezor-crypto

```bash
git submodule add https://github.com/trezor/trezor-crypto.git ThirdParty/trezor-crypto
```

Или клонировать в `ThirdParty/trezor-crypto`.

## 2. Файлы trezor-crypto для сборки

Добавьте в Makefile (или проект STM32CubeIDE):

```
ThirdParty/trezor-crypto/bip39.c
ThirdParty/trezor-crypto/bip32.c
ThirdParty/trezor-crypto/ecdsa.c
ThirdParty/trezor-crypto/secp256k1.c
ThirdParty/trezor-crypto/sha2.c
ThirdParty/trezor-crypto/hmac.c
ThirdParty/trezor-crypto/pbkdf2.c
ThirdParty/trezor-crypto/bignum.c
ThirdParty/trezor-crypto/rand.c
ThirdParty/trezor-crypto/base58.c
ThirdParty/trezor-crypto/address.c
ThirdParty/trezor-crypto/ripemd160.c
```

Плюс зависимости (см. `#include` в исходниках): `aes.c`, `ed25519-donna/*` (если нужен Ed25519).

## 3. Include-пути

```
-I$(TOP)/ThirdParty/trezor-crypto
-I$(TOP)/ThirdParty/trezor-crypto/ed25519-donna  # если используется
```

## 4. RNG (STM32H7)

В `main.c` или отдельном модуле:

```c
#include "stm32h7xx_hal.h"

RNG_HandleTypeDef hrng;

void MX_RNG_Init(void)
{
    hrng.Instance = RNG;
    hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
    if (HAL_RNG_Init(&hrng) != HAL_OK) Error_Handler();
}

void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng)
{
    __HAL_RCC_RNG_CLK_ENABLE();
}
```

В `stm32h7xx_hal_conf.h`: `#define HAL_RNG_MODULE_ENABLED`

## 5. Использование API

```c
#include "crypto_wallet.h"

uint8_t entropy[16];
uint8_t seed[64];
uint8_t priv_key[32];
uint8_t hash[32];
uint8_t sig[64];
char mnemonic[256];

crypto_rng_init();

/* 128 бит энтропии -> 12 слов */
random_buffer(entropy, 16);
crypto_entropy_to_mnemonic_12(entropy, mnemonic, sizeof(mnemonic));
memzero(entropy, 16);

/* Mnemonic -> seed (BIP-39) */
mnemonic_to_seed(mnemonic, "", seed, NULL);

/* Seed -> m/44'/0'/0'/0/0 */
crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key);
memzero(seed, 64);

/* Подпись хеша */
crypto_sign_btc_hash(priv_key, hash, sig);  /* priv_key обнуляется внутри */
```

## 6. Опции options.h

В `ThirdParty/trezor-crypto/options.h` или через `-D`:

- `USE_ETHEREUM 0` — если не нужен Ethereum
- `USE_KECCAK 1` — для некоторых адресов
- `USE_BIP32_CACHE 1` — кэш для ускорения

## 7. Безопасность

- Все приватные ключи, сиды, энтропия — обнуляются через `memzero` после использования
- `crypto_sign_btc_hash` обнуляет `priv_key` внутри
- Вызывающий код должен обнулять `entropy`, `seed` после вызова

## 8. Task Sign и get_wallet_seed

Таск подписи (`task_sign.c`) использует `get_wallet_seed(seed_out, max_len)` для получения BIP-39 seed.
Реализуйте эту функцию в модуле secure storage (например, зашифрованное хранилище или ввод мнемоники):

```c
int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    if (seed_out == NULL || max_len < 64) return -1;
    /* Загрузить seed из secure storage или mnemonic_to_seed() */
    return 0;
}
```

Слабый stub возвращает -1 (seed недоступен).
