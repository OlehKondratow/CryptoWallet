\page crypto_wallet "crypto_wallet: trezor-crypto glue (RNG/BIP32/ECDSA)"
\related crypto_hash_sha256
\related crypto_derive_btc_m44_0_0_0_0
\related crypto_sign_btc_hash
\related crypto_entropy_to_mnemonic_12
\related random_buffer

# `crypto_wallet.c` + `crypto_wallet.h`

<brief>Модуль `crypto_wallet` обёртывает trezor-crypto библиотеку: он поднимает STM32 RNG с энтропийным перемешиванием, предоставляет BIP-39 mnemonics, BIP-32 HDNode derivation и ECDSA secp256k1 подпись для стандартного пути Bitcoin m/44'/0'/0'/0/0.</brief>

## Краткий обзор
<brief>Модуль `crypto_wallet` обёртывает trezor-crypto библиотеку: он поднимает STM32 RNG с энтропийным перемешиванием, предоставляет BIP-39 mnemonics, BIP-32 HDNode derivation и ECDSA secp256k1 подпись для стандартного пути Bitcoin m/44'/0'/0'/0/0.</brief>

## Abstract (Synthèse логики)
`crypto_wallet` — это слой адаптации между проектом и trezor-crypto: с одной стороны, используется RNG (STM32 TRNG + софтверный entropy pool для стабильности), с другой — экспортируются "простые" API для вышестоящих уровней (sign_hash, derive_key, mnemonic_from_entropy). При `USE_CRYPTO_SIGN=0` все функции становятся заглушками, а вместо trezor идёт `sha256_minimal` для хеширования. Бизнес-роль — дать одну точку контроля для всей криптографии в проекте.

## Logic Flow (RNG + Crypto pipeline)

### RNG инициализация
1. `crypto_rng_init()` вызывается один раз из `main.c`.
2. Берёт текущий `HAL_GetTick()` и применяет LFSR-трансформацию для нелинейной mix-функции.
3. Результат XOR'ится в глобальный `s_rng_entropy_pool`.
4. Далее каждый вызов `random_buffer()` (из trezor-crypto):
   - пытается взять слово из STM32 TRNG (если `hrng.Instance` валиден),
   - XOR'ит с `s_rng_entropy_pool` и обновляет pool через LCG (мултипликативный конгруэнтный генератор).
   - Копирует результат в буфер вывода.

### Криптографический пути

**При `USE_CRYPTO_SIGN=1`:**
- BIP-39: мnemonic_from_data (16 байт -> 12 слов)
- BIP-32: hdnode_from_seed, затем цепочка derive операций для m/44'/0'/0'/0/0
- ECDSA: ecdsa_sign_digest с secp256k1, компактный формат r||s (64 байта)

**При `USE_CRYPTO_SIGN=0`:**
- Все функции возвращают -1 (не поддерживается)
- SHA-256 падает на `sha256_minimal`

## Прерывания/регистры
Модуль использует HAL RNG и `HAL_GetTick()` для энтропии, но прямых ISR/регистров не трогает. Единственное — в `crypto_sign_btc_hash()` после подписи вызывается `memzero()` для очистки приватного ключа.

## Тайминги и условия ветвления

| Функция | Когда работает | Ошибка |
|---|---|---|
| `crypto_rng_init` | один раз при старте (optional при USE_CRYPTO_SIGN=0) | -1 если hrng.Instance NULL |
| `random_buffer` | постоянно, под запросы trezor | no-op при NULL buf/len |
| `crypto_entropy_to_mnemonic_12` | при setup; читает 16 байт энтропии | -1 если USE_CRYPTO_SIGN=0 или bad buffer |
| `crypto_derive_btc_m44_0_0_0_0` | при подписании; берёт seed (64 байта) | -1 при ошибке derivation или USE_CRYPTO_SIGN=0 |
| `crypto_sign_btc_hash` | при подписании; берёт key+digest | -1 при ошибке sign или USE_CRYPTO_SIGN=0; очищает ключ после |
| `crypto_hash_sha256` | при формировании input для подписи | -1 при NULL input/output |

## Dependencies
Прямые зависимости:
- **trezor-crypto:** `bip39.h`, `bip32.h`, `ecdsa.h`, `secp256k1.h`, `rand.h`, `sha2.h` (при `USE_CRYPTO_SIGN`).
- **sha256_minimal.h** (fallback при `USE_CRYPTO_SIGN=0`).
- **memzero.h** для обнуления sensitive буферов.
- **HAL RNG:** `stm32h7xx_hal.h` и инициализация `hrng` из `hw_init.c`.

Глобальные:
- `s_rng_entropy_pool` (локальный статический пул),
- `hrng` (из `hw_init` по слабой ссылке),
- `random_buffer()` (hook, который trezor-crypto ожидает).

## Связи
- `task_sign.md` (consumer seed derivation + signing)
- `memzero.md` (key clearing)
- `sha256_minimal.md` (fallback hashing)
- `wallet_seed.md` (seed management)
- `hw_init.md` (RNG инициализация)
