# Plan testów: podpis Bitcoin i RNG

_Wygenerowano: patrz angielski `documentation/generated/testing-plan-signing-rng.md` — ten plik tłumaczenia może być nieaktualny po regeneracji._

Plan uzupełnia dokumentację projektu. Automatyczne hooki są minimalne; oznaczaj pozycje ręcznie w trackerze.

## 1. Zakres

- Ścieżka podpisu Bitcoin w stylu m/44'/0'/0'/0/0 (domyślnie w projekcie).
- Podpisywany komunikat: SHA-256(recipient|amount|currency), potem ECDSA secp256k1 kompaktowy podpis 64 bajty — **nie** surowa transakcja Bitcoin.
- RNG: STM32 TRNG + pul programowy (patrz `documentation/03-cryptography-and-signing.md`).

## 2. Podpis Bitcoin — wymagania wstępne

- Build: USE_CRYPTO_SIGN=1, USE_TEST_SEED=1 dla znanego wektora (tylko dev).
- Flash firmware; Ethernet lub WebUSB wg `documentation/04-http-and-webusb.md`.
- UART 115200 dla logów (zalecane).

## 3. Podpis Bitcoin — testy funkcjonalne

- **T-SIG-01** POST /tx poprawny JSON → UART: TX w kolejce, TX recv; wyświetlacz pending/sign.
- **T-SIG-02** Krótkie USER (PC13) → Confirm → UART: Signed OK (z seed testowym).
- **T-SIG-03** GET /tx/signed → JSON signed + hex sig 128 znaków (64 bajty).
- **T-SIG-04** Długie USER → Reject → brak poprawnego podpisu / Reject w logu.
- **T-SIG-05** Brak naciśnięcia 30 s → timeout.
- **T-SIG-06** Niepoprawny JSON / zły odbiorca → TX invalid.
- **T-SIG-07** WebUSB: `scripts/test_usb_sign.py` ping potem sign (USE_WEBUSB=1).
- **T-SIG-08** Cross-check: te same wejścia + ten sam seed → powtarzalny podpis (deterministyczne k tylko przy ścieżce RFC6979; zweryfikuj względem krypto projektu).

## 4. Podpis Bitcoin — bezpieczenie / negatywne

- **T-SEC-01** Bez USE_TEST_SEED i bez implementacji get_wallet_seed → brak seed po potwierdzeniu.
- **T-SEC-02** Brak klucza prywatnego lub seed w odpowiedziach UART/HTTP (inspekcja logów).

## 5. RNG — kontrole projektowe

- **T-RNG-01** HAL_RNG / TRNG włączony w buildzie; w konfiguracji produkcyjnej brak ścieżki tylko fallback.
- **T-RNG-02** Po resecie wyjście random_buffer różni się między cyklami zasilania (spot check).
- **T-RNG-03** Przegląd kodu: memzero na ścieżkach seed/privkey po użyciu (task_sign / crypto_wallet).

## 6. RNG — testy statystyczne DIEHARDER

- Cel: wykryć grube odchylenie / korelację; to nie certyfikacja krypto.
- **Firmware:** tylko surowe bajty na UART (bez tekstu). Ewentualnie USE_RNG_DUMP.
- **Plik przechwytu:** `scripts/capture_rng_uart.py --port ... --out rng.bin --bytes 134217728` (zalecane ≥128 MiB).
- **Uruchomienie:** `scripts/run_dieharder.py --file rng.bin` (wymaga pakietu `dieharder`).
- **Kryteria:** interpretuj wartości p; badaj skupiska niepowodzeń; przy granicznych — powtórz z większym plikiem.

## 7. Śledzenie

- Projekt: `documentation/03-cryptography-and-signing.md`, `04-http-and-webusb.md`, `06-integrity-rng-verification.md`.

## Lista kontrolna pliku DIEHARDER

[ ] Firmware wypuszcza **tylko** surowe bajty na wybranym interfejsie.
[ ] Baud UART zgodny ze skryptem (domyślnie 115200).
[ ] `scripts/capture_rng_uart.py` zapisał oczekiwaną liczbę bajtów.
[ ] `dieharder` zainstalowany: działa `dieharder -l`.
[ ] `scripts/run_dieharder.py --file rng.bin` zakończone; wyniki zarchiwizowane.
