\page trezor-crypto-integration "trezor-crypto: integracja i prymitywy kryptograficzne"

# Integracja trezor-crypto

<brief>CryptoWallet integruje trezor-crypto dla Bitcoin operacji kryptograficznych: generowanie mnemoniki BIP-39, pochodne klucze HD BIP-32, podpisywanie ECDSA secp256k1 z RFC6979, haszowanie SHA-256 i zarządzanie entropią poprzez STM32 TRNG + mieszanie puli entropii.</brief>

## Przegląd

Firmware CryptoWallet deleguje wszystkie operacje kryptograficzne do **trezor-crypto**, dobrze przetestowanej, produkcyjnej biblioteki kryptograficznej (licencja MIT), używanej w portfelach sprzętowych Trezor. Integracja jest warunkowa od flagi kompilacji `USE_CRYPTO_SIGN=1`; bez niego dostępny jest tylko SHA-256 przez `sha256_minimal.c`.

## Komponenty kryptograficzne

### 1. Generowanie mnemoniki BIP-39

**Moduł:** `bip39.c`

**Cel:** Konwersja 128-bitowej entropii na 12-słowną frazę mnemoniczną przy użyciu listy słów BIP-39 English.

**Implementacja w CryptoWallet:**
```c
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[16],
                                  char *mnemonic_out, size_t mnemonic_size);
```

**Przepływ:**
1. Generuj 128-bitową entropię z `random_buffer()` (STM32 TRNG + pula)
2. Wywołaj `mnemonic_from_data(entropy, 16, mnemonic_out)` z trezor
3. Wyjście: 12 słów oddzielonych spacją (np. "abandon wallet ... about")

**Przykład wektora testowego:**
- Entropia: wszystkie zera (nie rekomendowane w produkcji)
- Mnemonika: "abandon abandon abandon ... abandon about"
- Adres Bitcoin: `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`

### 2. Mnemonika BIP-39 → Ziarno

**Moduły:** `pbkdf2.c`, `hmac.c`

**Cel:** Konwersja mnemoniki + opcjonalna fraza hasła na 64-bajtowe ziarno dla pochodnych BIP-32.

**Implementacja:**
```c
// Otoczone w trezor; CryptoWallet wywołuje:
int mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                     uint8_t seed_out[64], size_t max_len);
```

**Parametry PBKDF2 (standard BIP-39):**
- HMAC-SHA512
- Iteracje: 2048
- Sól: prefiks "TREZOR" + fraza hasła

**Wyjście:** 64 bajty do użycia jako nasienie główne dla BIP-32.

### 3. Klucze hierarchiczne determiniczne BIP-32

**Moduły:** `bip32.c`, `curves.c`, `secp256k1.c`

**Cel:** Wyprowadź klucze potomne z ziarna głównego wzdłuż określonej ścieżki wyprowadzenia.

**Standardowa ścieżka w CryptoWallet:** `m/44'/0'/0'/0/0`

- `44` — BIP-44 (wielozadaniowe portfele HD)
- `0'` — Moneta Bitcoin (wzmacniana)
- `0'` — Konto 0 (wzmacniane)
- `0` — Łańcuch zewnętrzny
- `0` — Pierwszy adres

**Implementacja w CryptoWallet:**
```c
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32]);
```

**Proces wyprowadzenia:**
1. `hdnode_from_seed()` — Inicjalizuj główny HDNode ze 64-bajtowego ziarna
2. `hdnode_private_ckd()` — Wyprowadź na każdym wzmacnianym/niewzmacnianym kroku
3. `hdnode_fill_public_key()` — Oblicz klucz publiczny (opcjonalnie, do weryfikacji)
4. Wyjście: 32-bajtowy klucz prywatny (big-endian skalar)

**Bezpieczeństwo:** Wzmacniane wyprowadzenie (`'` notacja) zapewnia, że klucze potomne nie mogą być wyprowadzone z samego klucza publicznego.

### 4. Podpisywanie ECDSA (secp256k1)

**Moduły:** `ecdsa.c`, `secp256k1.c`, `bignum.c`, `rfc6979.c`

**Cel:** Podpisz skrót wiadomości przy użyciu ECDSA z krzywą eliptyczną secp256k1 (standard Bitcoin).

**Implementacja w CryptoWallet:**
```c
int crypto_sign_btc_hash(uint8_t priv_key[32],
                        const uint8_t hash[32],
                        uint8_t sig_out[64]);
```

**Algorytm podpisywania:**
- **Krzywa:** secp256k1 (p = 2^256 - 2^32 - 977)
- **Algorytm skrótu:** SHA-256
- **RFC6979:** Deterministyczne generowanie k (bez zależności RNG)
- **Wyjście:** 64-bajtowy kompaktowy podpis (r || s, oba 32-bajtowe big-endian)

**Deterministyczne k (korzyści RFC6979):**
- Ten sam klucz prywatny + ta sama wiadomość = identyczny podpis
- Zapobiega atakom na niepowodzenie RNG
- Umożliwia weryfikację podpisu w testach

### 5. Haszowanie SHA-256

**Moduły:** `sha2.c`, `hasher.c`

**Cel:** Haszuj dowolne dane przy użyciu SHA-256 (FIPS 180-4).

**Implementacja w CryptoWallet:**
```c
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[32]);
```

**Dane wejściowe:** Kanoniczny ciąg transakcji sformatowany jako `"recipient|amount|currency"`

**Przykład:**
- Wejście: `"1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA|0.5|BTC"`
- Wyjście: 32-bajtowy skrót SHA-256

**Fallback:** Gdy `USE_CRYPTO_SIGN=0`, deleguje do `sha256_minimal.c`.

### 6. Walidacja adresu (Base58Check)

**Moduły:** `base58.c`, `address.c`

**Cel:** Waliduj adresy Bitcoin przy użyciu kodowania Base58Check (weryfikacja sumy kontrolnej).

**Użycie w CryptoWallet:** `tx_request_validate.c` wywołuje `base58_decode_check()` z trezor.

**Obsługiwany format:** Starsze adresy P2PKH (zaczynające się z '1', 25 bajtów razem).

### 7. Entropia i generowanie liczb losowych

**Moduły:** `rand.c`, `hmac_drbg.c`

**Integracja STM32H743 TRNG:**
```c
int crypto_rng_init(void);  // Wywoływane raz z main.c
void random_buffer(uint8_t *buf, size_t len);  // Wywoływane przez trezor-crypto
```

**Źródła entropii:**
1. **Sprzętowy TRNG:** `HAL_RNG_GenerateRandomNumber()` → 32-bitowe słowa
2. **Pula entropii:** Mieszanie programowego LCG (XOR + aktualizacja stanu)
3. **Kombinacja:** `rng_val = HAL_RNG_value XOR entropy_pool`

**Parametry LCG (Numerical Recipes):**
- Mnożnik: `1664525`
- Przyrost: `1013904223`
- Aktualizacja stanu: `pool = (pool * 1664525) + 1013904223`

**Jakość:** Łączy sprzętową losowość z wybieleniem oprogramowania, aby obronić się przed stronniczością TRNG.

## Kompletny potok podpisywania

```
Wejście sieciowe (HTTP POST /tx lub WebUSB)
    ↓
[tx_request_validate.c]
    ├─ Dekodowanie Base58Check: adres odbiorcy
    ├─ Regex: format kwoty (dziesiętny)
    ├─ Whitelist: waluta ("BTC", "ETH", ...)
    ↓
[task_sign.c → crypto_wallet.c]
    ├─ build_hash_input(): Format "recipient|amount|currency"
    ├─ crypto_hash_sha256(): 32-bajtowy skrót
    ├─ Potwierdzenie użytkownika: g_user_event_group (naciśnięcie przycisku)
    ├─ get_wallet_seed(): 64-bajtowe ziarno BIP-39
    ├─ crypto_derive_btc_m44_0_0_0_0(): 32-bajtowy klucz prywatny
    ├─ crypto_sign_btc_hash(): 64-bajtowy podpis (r||s)
    ├─ memzero(): Wyczyść poufne bufory
    ↓
[Wyjście]
    ├─ USB: WebUSB_NotifySignatureReady(64-bajtowy podpis)
    ├─ Sieć: odpowiedź HTTP JSON z szyfrowanym hex podpisem
    ↓
Ukończone
```

## Opcje kompilacji

### `USE_CRYPTO_SIGN=1` (Domyślnie dla produkcji)

**Połączone moduły (22 razem):**
- **BIP:** bip39, bip32
- **ECDSA:** ecdsa, secp256k1, curves, bignum, rfc6979
- **Hash:** sha2, hasher, sha3, blake256, blake2b, blake2s, groestl
- **HMAC:** hmac, hmac_drbg, pbkdf2
- **Kodowanie:** base58, address, ripemd160
- **Inne:** rand, nist256p1
- **ED25519:** (połączone, ale nie używane w obecnym CryptoWallet)

**Rozmiar binarny:** ~100-150 KB kodu obiektu

**Cechy:**
- Pełne podpisywanie ECDSA
- Pochodne BIP-39/BIP-32
- STM32 TRNG + pula entropii
- Deterministyczne RFC6979 k

### `USE_CRYPTO_SIGN=0` (Minimalna kompilacja)

**Połączone moduły:**
- `sha2.c` (tylko SHA-256 z trezor-crypto)
- `sha256_minimal.c` (fallback, ~16 KB)

**Rozmiar binarny:** ~5 KB

**Cechy:**
- Tylko haszowanie SHA-256
- Brak ECDSA, BIP-39, BIP-32
- Brak wyprowadzenia klucza

**Przypadek użycia:** Testowanie, walidacja interfejsu użytkownika, minimalne systemy wbudowane.

### `USE_TEST_SEED=1` (Tylko opracowanie)

**Zachowanie:**
- `get_wallet_seed()` zwraca zakodowaną mnemonikę testową BIP-39
- Mnemonika: "abandon abandon ... about"
- Wyprowadzony adres Bitcoin: `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`
- Umożliwia powtarzalne podpisy dla testów automatycznych

**⚠️ OSTRZEŻENIE:** Nigdy nie używaj w produkcji lub z rzeczywistymi środkami.

## Zagadnienia bezpieczeństwa

### 1. Jakość entropii

**Mieszanie z wielu źródeł:**
- STM32H743 sprzętowy TRNG zapewnia losowość fizyczną
- Pula entropii oprogramowania dodaje zmienność czasową
- Połączone poprzez XOR, aby obronić się przed awariąą jednego źródła

**Walidacja:**
- Skrypty: `scripts/capture_rng_uart.py`, `scripts/run_dieharder.py`
- Testy statystyczne NIST FIPS 140-2 są rekomendowane

### 2. Obsługa materiału klucza

**Zerowanie:**
- Wszystkie poufne bufory (ziarno, klucz prywatny, skrót) czyszczone przez `memzero()`
- Używa zmiennych zapisów, aby zapobiec optymalizacji kompilatora
- Klucze nigdy nie są logowane, wyświetlane ani przesyłane niezaszyfrowane

**Czas życia bufora:**
- Klucze prywatne przydzielone na stosie (zakres lokalny)
- Czyszczone przed powrotem funkcji
- Brak globalnego przechowywania kluczy w firmware

### 3. Determinizm podpisu

**Korzyść RFC6979:**
- Usuwa zależność RNG z generacji ECDSA `k`
- Zapobiega przypadkowemu ponownemu użyciu klucza (ten sam skrót → ten sam podpis)
- Obsługuje deterministyczne wektory testowe

**Trade-off:** Podpis jest deterministyczny, nie losowy (standard w Bitcoin).

### 4. Bezpieczeństwo mnemoniki

**Ochrona:**
- Ziarno BIP-39 obliczane na żądanie, nigdy nie przechowywane
- Natychmiast wyzerowane po wyprowadzeniu klucza
- Lista mnemoniczna nie przechowywana w pamięci flash firmware

**Implementacja dla produkcji:**
- Wdróż bezpieczny element (SE) lub zaszyfrowaną flash do przechowywania ziaren
- Używaj KDF z dodatkową frazą hasła użytkownika
- Rozważ integrację portfela sprzętowego

## Zależności i przypisanie

**Biblioteka:** [trezor-crypto](https://github.com/trezor/trezor-crypto)
- **Licencja:** MIT
- **Wersja:** Zintegrowana jako podprojekt w `ThirdParty/trezor-crypto/`
- **Opiekunowie:** SatoshiLabs (projekt Trezor)

**Powiązana dokumentacja:**
- BIP-32: [Bitcoin Improvement Proposal 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)
- BIP-39: [Bitcoin Improvement Proposal 39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki)
- BIP-44: [Bitcoin Improvement Proposal 44](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki)
- RFC6979: [Deterministic ECDSA](https://tools.ietf.org/html/rfc6979)
- FIPS 180-4: SHA kryptograficzne funkcje skrótu

## Powiązane moduły w CryptoWallet

- `crypto_wallet.md` — Wrapper API i inicjalizacja RNG
- `task_sign.md` — Podpisywanie FSM, które wywołuje funkcje crypto
- `memzero.md` — Bezpieczne zerowanie bufora
- `wallet_seed.md` — Test seed stub (tylko opracowanie)
- `tx_request_validate.md` — Walidacja adresu i sanityzacja danych wejściowych
