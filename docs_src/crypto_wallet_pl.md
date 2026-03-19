\page crypto_wallet "crypto_wallet: adapter trezor-crypto (RNG/BIP32/ECDSA)"
\related crypto_hash_sha256
\related crypto_derive_btc_m44_0_0_0_0
\related crypto_sign_btc_hash
\related crypto_entropy_to_mnemonic_12
\related random_buffer

# `crypto_wallet.c` + `crypto_wallet.h`

<brief>Moduł `crypto_wallet` wrapuje bibliotekę trezor-crypto: aktywuje RNG STM32 z mieszaniem entropii, udostępnia mnemonickę BIP-39, derywację HDNode BIP-32 oraz podpisywanie ECDSA secp256k1 dla standardowej ścieżki Bitcoin m/44'/0'/0'/0/0.</brief>

## Przegląd

Moduł `crypto_wallet` jest warstwą adaptacji między projektem a trezor-crypto: z jednej strony używany jest RNG (STM32 TRNG + bufor entropii softwarowy dla stabilności), z drugiej strony eksportowane są "proste" API dla wyższych poziomów (sign_hash, derive_key, mnemonic_from_entropy). Gdy `USE_CRYPTO_SIGN=0`, wszystkie funkcje stają się zatyczkami, a zamiast trezor używany jest `sha256_minimal` do mieszania. Rola biznesowa: zapewnić jeden punkt kontroli dla całej kryptografii w projekcie.

## Przepływ logiki (RNG + Crypto pipeline)

### Inicjalizacja RNG

1. `crypto_rng_init()` jest wywoływana raz z `main.c`.
2. Pobiera bieżący `HAL_GetTick()` i stosuje transformację LFSR dla nieliniowej funkcji mix.
3. Wynik jest XORowany do globalnego `s_rng_entropy_pool`.
4. Następnie każde wywołanie `random_buffer()` (z trezor-crypto):
   - próbuje pobrać słowo z STM32 TRNG (jeśli `hrng.Instance` jest ważny)
   - XORuje z `s_rng_entropy_pool` i aktualizuje pool poprzez LCG (multiplikatywny generator kongruentny)
   - kopiuje wynik do bufora wyjściowego

### Ścieżki kryptograficzne

**Gdy `USE_CRYPTO_SIGN=1`:**
- BIP-39: mnemonic_from_data (16 bajtów → 12 słów)
- BIP-32: hdnode_from_seed, następnie łańcuch operacji derywacji dla m/44'/0'/0'/0/0
- ECDSA: ecdsa_sign_digest z secp256k1, format zwarty r||s (64 bajty)

**Gdy `USE_CRYPTO_SIGN=0`:**
- Wszystkie funkcje zwracają -1 (nieobsługiwane)
- SHA-256 wraca do `sha256_minimal`

## Przerwania i rejestry

Moduł używa HAL RNG i `HAL_GetTick()` dla entropii, ale nie dotyka bezpośrednich ISR/rejestrów. Jedynym szczegółem — w `crypto_sign_btc_hash()` po podpisywaniu wywoływana jest `memzero()` do czyszczenia klucza prywatnego.

## Czasy i warunki rozgałęzienia

| Funkcja | Kiedy pracuje | Błąd |
|---------|---|---|
| `crypto_rng_init` | raz przy starcie (opcjonalne gdy USE_CRYPTO_SIGN=0) | -1 jeśli hrng.Instance NULL |
| `random_buffer` | stale na żądania trezor | no-op jeśli NULL buf/len |
| `crypto_entropy_to_mnemonic_12` | przy setup; czyta 16 bajtów entropii | -1 jeśli USE_CRYPTO_SIGN=0 lub zły bufor |
| `crypto_derive_btc_m44_0_0_0_0` | przy podpisywaniu; pobiera seed (64 bajty) | -1 przy błędzie derywacji lub USE_CRYPTO_SIGN=0 |
| `crypto_sign_btc_hash` | przy podpisywaniu; pobiera key+digest | -1 przy błędzie podpisu lub USE_CRYPTO_SIGN=0; czyści klucz po |
| `crypto_hash_sha256` | gdy formuje input dla podpisu | -1 jeśli NULL input/output |

## Zależności

Bezpośrednie zależności:
- **trezor-crypto:** `bip39.h`, `bip32.h`, `ecdsa.h`, `secp256k1.h`, `rand.h`, `sha2.h` (gdy `USE_CRYPTO_SIGN`)
- **sha256_minimal.h** (fallback gdy `USE_CRYPTO_SIGN=0`)
- **memzero.h** do czyszczenia wrażliwych buforów
- **HAL RNG:** `stm32h7xx_hal.h` i inicjalizacja `hrng` z `hw_init.c`

Globalne:
- `s_rng_entropy_pool` (lokalny statyczny bufor)
- `hrng` (z `hw_init` poprzez słabe odwołanie)
- `random_buffer()` (hook, który trezor-crypto oczekuje)

## Relacje modułów

- `task_sign.md` (konsument derywacji seed + podpisywania)
- `memzero.md` (czyszczenie klucza)
- `sha256_minimal.md` (fallback mieszania)
- `wallet_seed.md` (zarządzanie seed)
- `hw_init.md` (inicjalizacja RNG)
