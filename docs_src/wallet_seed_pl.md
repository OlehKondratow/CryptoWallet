\page wallet_seed "wallet_seed: test seed (development only)"
\related get_wallet_seed

# `wallet_seed.c`

<brief>Moduł `wallet_seed` to stub dla `get_wallet_seed()` gdy `USE_TEST_SEED=1`: zwraca BIP-39 seed z znanego testowego mnemonika "abandon...about", **nigdy dla rzeczywistych funduszy**.</brief>

## Przegląd

<brief>Moduł `wallet_seed` to stub dla `get_wallet_seed()` gdy `USE_TEST_SEED=1`: zwraca BIP-39 seed z znanego testowego mnemonika "abandon...about", **nigdy dla rzeczywistych funduszy**.</brief>

## Abstrakcja (synteza logiki)

Do nauki i testowania potrzebne jest powtarzalne nasiono. `wallet_seed` zawiera wbudowany standardowy wektor testowy BIP-39 (adres `1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA`), pozwalając na reprodukcję podpisów i weryfikację ścieżki podpisywania. To jest **development-only** — produkcja musi zastąpić to bezpiecznym elementem lub zaszyfrowaną pamięcią flash. Rola biznesowa: zapewnić deterministyczne nasiono podczas rozwoju.

## Przepływ logiki

1. Gdy `USE_CRYPTO_SIGN=1` i `USE_TEST_SEED=1`, kompilowana jest prawdziwa implementacja.
2. Funkcja `get_wallet_seed()` po prostu wywoła `mnemonic_to_seed()` z trezor-crypto ze znanym ciągiem.
3. Kopie wynikowy (64 bajty) do buforu wyjścia i czyści lokalne nasiono poprzez `memzero()`.
4. Pod każdą inną flagą — kompilowany jest pusty stub, a `get_wallet_seed()` (słaby symbol w `task_sign.c`) po prostu zwraca -1.

## Przerwania/rejestry

Brak ISR lub rejestrów: tylko transformacja ciągu i memcpy.

## Czasy i warunki rozgałęzienia

| Kompilacja | Zachowanie |
|---|---|
| `USE_CRYPTO_SIGN=1` i `USE_TEST_SEED=1` | Zwraca seed z "abandon...about" |
| Inny | Weak stub: -1 (nie zaimplementowane) |

## Zależności

- `bip39.h` z trezor-crypto (`mnemonic_to_seed`)
- `memzero.h` do czyszczenia lokalnego buforu
- Używane z `task_sign.c` podczas podpisywania

## Relacje

- `task_sign.md` (konsument w potoku podpisywania)
- `crypto_wallet.md` (opakowanie nad trezor)
- `memzero.md` (czyszczenie nasiona)
