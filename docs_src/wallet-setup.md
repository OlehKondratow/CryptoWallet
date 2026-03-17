# Добавление кошелька в CryptoWallet

Кошелёк выводится из BIP-39 seed по пути `m/44'/0'/0'/0/0` (Bitcoin). Seed получается через функцию `get_wallet_seed()`.

## Быстрый тест (USE_TEST_SEED=1)

Для ручной проверки подписи используйте встроенный тестовый seed:

```bash
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 minimal-lwip
```

Мнемоника: `abandon abandon ... about` (BIP-39 test vector). Адрес: `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`. **Только для dev, не для реальных средств.**

## Текущая реализация

`get_wallet_seed()` объявлена как **weak symbol** в `task_sign.c`:

```c
__attribute__((weak)) int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    return -1;  /* No seed — implement in secure storage */
}
```

Чтобы подпись работала, нужно предоставить **сильную** реализацию в отдельном модуле.

## Вариант 1: Мнемоника из конфигурации (для разработки)

Создайте модуль `wallet_seed.c` с тестовой мнемоникой. **Только для dev-тестов, не для реальных средств.**

```c
/* wallet_seed.c — ТОЛЬКО ДЛЯ ТЕСТИРОВАНИЯ */
#include "bip39.h"
#include <string.h>

int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    if (seed_out == NULL || max_len < 64) return -1;

    /* Тестовая мнемоника (BIP-39 test vector, пустой passphrase) */
    const char *mnemonic = "abandon abandon abandon abandon abandon abandon "
                           "abandon abandon abandon abandon abandon about";

    uint8_t seed[64];
    mnemonic_to_seed(mnemonic, "", seed, NULL);
    memcpy(seed_out, seed, 64);
    memzero(seed, sizeof(seed));
    return 0;
}
```

Добавьте в Makefile в `OBJ_MINIMAL_LWIP` при `USE_CRYPTO_SIGN=1`:

```makefile
$(BUILD)/wallet_seed.o: $(TOP)/Core/Src/wallet_seed.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
```

## Вариант 2: Seed из защищённого хранилища

Для продакшена seed должен храниться в защищённой области:

- **OTP** (One-Time Programmable) — один раз записать при производстве
- **Encrypted Flash** — зашифрованный seed в Flash с ключом из OTP/HSM
- **Secure Element** — внешний чип (ATECC608, etc.)

Пример заглушки:

```c
int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    if (seed_out == NULL || max_len < 64) return -1;

    /* Загрузить из OTP/Flash/Secure Element */
    if (secure_storage_read(SEED_SLOT, seed_out, 64) != 0)
        return -1;
    return 0;
}
```

## Вариант 3: Ввод мнемоники при первом запуске

1. Пользователь вводит 12/24 слова (кнопки, дисплей, UART).
2. `mnemonic_check(mnemonic)` — проверка.
3. `mnemonic_to_seed(mnemonic, passphrase, seed, NULL)`.
4. Сохранить seed в зашифрованном виде или вывести ключ и попросить сохранить.

## Структура trezor-crypto

| Функция                | Файл   | Описание                          |
|------------------------|--------|-----------------------------------|
| `mnemonic_from_data`   | bip39.c| 16 байт entropy → 12 слов         |
| `mnemonic_to_seed`     | bip39.c| мнемоника + passphrase → 64 байт |
| `mnemonic_check`       | bip39.c| проверка checksum                 |
| `hdnode_from_seed`      | bip32.c| seed → HD node                    |
| `hdnode_private_ckd`   | bip32.c| derive по пути                   |

## Адрес для тестовой мнемоники

Мнемоника `abandon abandon ... about` даёт первый Bitcoin-адрес:

```
m/44'/0'/0'/0/0 → 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA
```

Проверка: [iancoleman.io/bip39](https://iancoleman.io/bip39) или trezor-crypto tools.
