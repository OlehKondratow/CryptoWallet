# 3. Kryptografia i podpisywanie

## 3.1 Cele

- Pochodzenie kluczy z mnemonic BIP-39 (gdy podpis włączony), BIP-32 dla ścieżek kont.
- Podpisywanie transakcji w stylu Bitcoin **ECDSA secp256k1** (stos trezor-crypto).
- Użycie sprzętowego TRNG STM32 tam, gdzie stos oczekuje losowych bajtów; tam gdzie jest walidacja — zamknięcie przy błędzie.

## 3.2 Tryby kompilacji

| `USE_CRYPTO_SIGN` | Zachowanie |
|-------------------|------------|
| `1` | trezor-crypto: RNG, BIP-39/32, ECDSA, powiązane skróty |
| `0` | Okrojona ścieżka (np. minimalny SHA-256) — **nie** konfiguracja portfela sprzętowego |

`USE_TEST_SEED=1` wymusza **znaną** mnemonic testową — **tylko** lab/CI; klucze są przewidywalne.

## 3.2b Pochodzenie klucza prywatnego (jak powstają klucze)

- **Wejście seed:** `get_wallet_seed()` wypełnia **64-bajtowy seed BIP-39** (lub zwraca błąd). W zadaniu podpisu **nie ma** osobnej ścieżki „tylko RNG bez seed” — korzeniem jest seed.
- **Pochodzenie:** `crypto_derive_btc_m44_0_0_0_0()` w `Core/Src/crypto_wallet.c` wywołuje trezor-crypto: `hdnode_from_seed()`, potem ścieżka **BIP-32 `m/44'/0'/0'/0/0`** (domyślny Bitcoin w projekcie).
- **Determinizm:** przy **stałym** seed klucz prywatny secp256k1 na tej ścieżce jest **w pełni wyznaczony** — pochodzenie **nie zużywa** TRNG. Zmiana ścieżki lub seed zmienia klucz.
- **Gdzie TRNG nadal ma znaczenie:** podpis **ECDSA** wymaga losowości (wewnętrzności biblioteki mogą wołać RNG); **tworzenie nowej mnemonic** z entropii — `crypto_entropy_to_mnemonic_12()` z **128 bitami entropii** (wywołujący musi dostarczyć entropię, np. z TRNG w pełnym portfelu). Ścieżka **`task_sign`** tylko **ładuje** seed i wyprowadza klucz — **nie** implementuje end-to-end „utwórz nowy portfel”.

## 3.3 Potok podpisu (pojęciowo)

1. Ingress (HTTP `POST /tx` lub WebUSB) parsuje odbiorcę/kwotę/walutę.
2. `tx_request_validate()` egzekwuje format i politykę.
3. Żądanie trafia do kolejki `task_sign`; użytkownik potwierdza przyciskiem.
4. `crypto_wallet` wyprowadza klucze i podpisuje; odpowiedź przez `GET /tx/signed` lub zakończenie WebUSB.

**Uwaga HTTP:** handler może zwrócić **200** z HTML mimo błędu walidacji — klient musi odpytywać status podpisu lub ufać OLED.

## 3.4 Higiena pamięci

- **`memzero()`** — zerowanie wrażliwych buforów zapisami volatile; kompilator nie powinien usuwać (implementacja).
- **Stosy** — stałe rozmiary w konfiguracji FreeRTOS; na gorącej ścieżce podpisu bez nieograniczonego alokowania.

## 3.4b Przechowywanie seed / kluczy prywatnych (uczciwy zakres)

- **Hak provisioning:** `get_wallet_seed()` (patrz `task_sign.c`). Domyślna słaba implementacja zwraca **błąd** — **brak** wbudowanego „sejfu” z trwałym przechowywaniem kluczy w repozytorium.
- **`USE_TEST_SEED=1`:** `wallet_seed.c` wstrzykuje **znaną** mnemonic testową — tylko lab/CI.
- **Produkcja** (z komentarzy w kodzie): zastąpić secure element, szyfrowany flash lub inną modelowaną groźbę — poza minimalną integracją.
- **Rozróżnienie:** klucze podpisu **obrazu** bootloadera (`stm32_secure_boot`) i klucze **portfela** (BIP-32/39) to **inne** sekrety i domeny zaufania.

## 3.5 TRNG i przechowywanie seed związane z płytą

Łączy **sprzętową losowość**, **weryfikację laboratoryjną** i **przyszły** projekt **zaszyfrowanego seed w spoczynku** związanego z płytą. Wiązanie przechowywania **nie jest zaimplementowane**; TRNG **jest** używany przez stos krypto tam gdzie włączone.

### 3.5.1 TRNG w firmware (dziś)

- **Sprzętowy RNG** (`HAL_RNG` / TRNG STM32) zasila **trezor-crypto** i powiązany kod przy włączonym podpisie; pule i użycie API według oczekiwań biblioteki.
- **Rola:** entropia dla krypto w czasie działania (nonce, ścieżki generacji kluczy w bibliotece itd.) — **nie** to samo co „tryb zrzutu UART” poniżej.

### 3.5.2 Tryb zrzutu UART (tylko lab / CI)

- **`USE_RNG_DUMP=1`:** dedykowane zadanie wysyła **surowe** bajty TRNG na UART do **zewnętrznych testów statystycznych** (np. capture + dieharder). **CWUP** nie działa na tym UART w tym trybie.
- **Nie** traktuj strumienia UART jako ścieżki produkcyjnej dla **zapisu** kluczy w pamięci: w produkcji wołaj driver RNG **w firmware**. Szczegóły i CI: [`06-integrity-rng-verification.md`](06-integrity-rng-verification.md).

### 3.5.3 TRNG a UID w przyszłym szyfrowanym magazynie

| Wejście | Typowa rola |
|---------|-------------|
| **TRNG (sprzętowy)** | **Entropia** dla wartości jednorazowych: sól instalacji obok szyfrogramu, nonce AEAD, dowolny losowy materiał nie do przewidzenia ani skopiowania z innej jednostki. |
| **UID (96-bit, fabryczny)** | **Wiązanie:** stabilna wartość na krzem mieszana w **KDF**, aby szyfrogram + PIN z **tej** flash **nie** deszyfrowały się na **innym** MCU. **Nie** sekret — patrz limity poniżej. |

Poprawny projekt używa **obu**: losowość z **TRNG**; **UID** (i PIN) przeciw **przenośnym** klonom flash. **Nie** używać UID jako jedynej „entropii” do opakowania seed.

### 3.5.4 Seed na dysku związany z urządzeniem (pomysł — nie zaimplementowano)

**Zamiar:** utrwalać **zaszyfrowany** seed we wewnętrznej lub zewnętrznej flash, ale **wiązać** odszyfrowanie z **tą** płytą, aby ten sam szyfrogram **nie** mógł być odblokowany na innym urządzeniu — nawet przy znanym PIN i kopii flash.

**Szkic:**

1. **Unikalne wejście:** odczyt **UID** (rejestry STM32 `UID`), plus **losowa sól** przy pierwszej konfiguracji (**z TRNG**, §3.5.3) zapisana obok szyfrogramu.
2. **Pochodzenie klucza:** `K = KDF(PIN || hardware_salt || UID || stored_salt)` (algorytm i liczba iteracji — osobny wybór).
3. **W spoczynku:** tylko **szyfrogram AEAD + metadane** — **nigdy** surowy seed.
4. **Odblokowanie:** użytkownik wpisuje PIN; firmware przelicza `K` z **lokalnym** UID; na innym chipie deszyfrowanie się nie udaje.

**Co pomaga:** sklonowany flash na innej płycie; przypadkowe przywrócenie dumpu bez jawnego eksportu.

**Czego nie rozwiązuje:** UID nie jest ukrytym root-of-trust; kradzież **tego** urządzenia + PIN nadal odszyfrowuje; przeniesienie portfela na nowe urządzenie wymaga **jawnego** backupu/eksportu.

**Relacja do innych opcji:** zgodne z secure element, szyfrowanym flash lub układem Trezor (PIN + zaszyfrowany DEK) — wiązanie płyty jako **dodatkowe** wejście KDF.

## 3.6 Czego ten dokument **nie** obejmuje

- Algorytmy podpisu bootloadera i przechowywanie kluczy w `stm32_secure_boot` — patrz tamto repozytorium.
- Formalna analiza kanałów pobocznych — tu nie jest twierdzona.
