\page sha256_minimal "sha256_minimal: compact SHA-256 (no trezor-crypto)"
\related sha256_minimal

# `sha256_minimal.c` + `sha256_minimal.h`

<brief>Moduł `sha256_minimal` to kompaktna implementacja SHA-256 (public domain), używana gdy `USE_CRYPTO_SIGN=0`, gdy trezor-crypto nie jest linkowany, ale hashing jest potrzebny do UI/logowania.</brief>

## Przegląd

<brief>Moduł `sha256_minimal` to kompaktna implementacja SHA-256 (public domain), używana gdy `USE_CRYPTO_SIGN=0`, gdy trezor-crypto nie jest linkowany, ale hashing jest potrzebny do UI/logowania.</brief>

## Abstrakcja (synteza logiki)

`sha256_minimal` to samodzielna implementacja SHA-256, niezależna od wszystkiego, pozwalająca minimalnej kompilacji (bez trezor-crypto) mieć przynajmniej podstawowy hashing do celów debugowania i UI. To **nie** jest przeznaczone do produkcyjnego podpisywania (ponieważ brak ECDSA), ale raczej do symulacji/nauki. Rola biznesowa: zapewnić fallback przy usuwaniu warstwy crypto.

## Przepływ logiki

1. **Inicjalizacja:** standardowe wartości H SHA-256 do tablicy `state[8]`
2. **Główna pętla:** przetwarzaj dane w blokach 64-bajtowych
3. **Padding:** dodaj flagę 0x80, zera i długość w bitach (per SHA-256 spec)
4. **Transform:** dla każdego bloku 64-bajtowego:
   - Rozwin 16 słów do W[0..63] poprzez transformacje SIG0/SIG1
   - 64 rundy z rotatorami i logiką mająt (CH, MAJ, EP0, EP1)
5. **Finalizacja:** wyjście 32 bajtów stanu w big-endian

## Przerwania/rejestry

Brak ISR lub rejestrów: czyste obliczenia.

## Czasy i warunki rozgałęzienia

| Etap | Co się dzieje |
|---|---|
| Input | Może być dowolne len (w tym 0) |
| Padding | Jeśli len > 56 po dodaniu flagi, wymaga dodatkowego bloku transform |
| Output | Zawsze 32 bajty w big-endian |

## Zależności

- `<stdint.h>`, `<string.h>` (memcpy, memset)
- Używane z `crypto_wallet.c` gdy `USE_CRYPTO_SIGN=0`

## Relacje

- `crypto_wallet.md` (dostawca fallback)
- `task_sign.md` (konsument w minimalnej kompilacji)
