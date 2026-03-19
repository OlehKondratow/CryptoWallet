\page task_sign "task_sign: potok podpisywania transakcji + FSM potwierdzenia"
\related Task_Sign_Create
\related g_tx_queue
\related g_user_event_group
\related g_last_sig_ready

# `task_sign.c` + `task_sign.h`

<brief>Moduł `task_sign` implementuje główny potok podpisywania: odbiera zwalidowany żądanie z kolejki, tworzy deterministyczne wejście dla SHA-256, czeka na potwierdzenie/odrzucenie użytkownika i po sukcesie przechowuje zwartą sygnaturę oraz przygotowuje ją do odpowiedzi sieciowej/USB.</brief>

## Przegląd

Moduł `task_sign` jest centralnym punktem "polityki i kryptografii" w projekcie: łączy dane sieciowe z potwierdzeniem człowieka i operacją kryptograficznego podpisania. Jego rola biznesowa to gwarancja, że operacje kryptograficzne nie zaczynają się przed walidacją ciągu wejściowego i nie kończą się bez wyraźnego zdarzenia `Confirm` z `task_user`. W przypadku błędów przechodzi system do bezpiecznego trybu UX (via `g_security_alert` i aktualizacja kontekstu wyświetlania).

## Przepływ logiki (jawna FSM podpisywania)

Logika wewnętrzna jest zorganizowana poprzez enum `signing_state_t` i zmienną lokalną `state`. Stany semantyczne i przejścia:

### 1) SIGNING_IDLE

Kroki:
1. Czekaj na transakcję z `g_tx_queue` (timeout 200ms).
2. Waliduj ładunek hosta poprzez `tx_request_validate`.
3. Na błędzie walidacji: zaloguj, ustaw `g_security_alert=1`, wyczyść tymczasowe bufory, wróć do `IDLE`.
4. Na sukces: przejdź do `SIGNING_RECEIVED`.

### 2) SIGNING_RECEIVED

Kroki:
1. Zaktualizuj kontekst wyświetlania (waluta/amount, bezpieczny locked=false podczas oczekiwania na potwierdzenie).
2. Utwórz deterministyczne wejście SHA-256 z pól recipient/amount/currency.
3. Oblicz SHA-256 (błąd → security_alert=1 i powrót do IDLE).
4. Przejdź do `SIGNING_WAIT_CONFIRM`.

### 3) SIGNING_WAIT_CONFIRM

Kroki:
1. Wyczyść bity grupy zdarzeń `EVENT_USER_CONFIRMED | EVENT_USER_REJECTED`.
2. Czekaj na jedno ze zdarzeń z timeoutem `CONFIRM_TIMEOUT_MS=30000ms`.
3. Gałęzie:
   - `REJECTED` → zaloguj "Rejected", wyczyść, wróć do IDLE
   - brak `CONFIRMED` (timeout) → zaloguj "Timeout", wyczyść, wróć do IDLE
4. Na potwierdzenie:
   - otrzymaj seed (`get_wallet_seed`) i derywuj klucz m/44'/0'/0'/0/0
   - sygnatura kryptograficzna `crypto_sign_btc_hash(...)`
   - przechowaj `g_last_sig` i ustaw `g_last_sig_ready=1`
   - wyzeruj bufory wrażliwe
   - zresetuj `g_security_alert=0`
   - zaktualizuj kontekst wyświetlania na locked/valid i wyczyść flagę pending poprzez `UI_ClearPending()`

## Przerwania i rejestry

Brak bezpośrednich operacji ISR/rejestrów. Jednakże istnieje krytyczny aspekt bezpieczeństwa:
- Bufory wrażliwe są czyszczone poprzez `memzero()` na wszystkich ścieżkach wyjścia (łącznie z gałęziami błędów)
- Kluczowe "punkty" bezpiecznego zachowania są powiązane z grupami zdarzeń i kolejkami

## Czasy i warunki rozgałęzienia

| Moment | Wartość | Co kontroluje |
|--------|---------|---|
| czekaj na ładunek z kolejki | 200ms | unikaj blokowania na zawsze na pustej kolejce |
| czekaj na mutex kontekstu wyświetlania | 50ms | unikaj zawieszania się przy problemach z mutexem UI |
| potwierdzenie użytkownika | 30000ms | timeout odmawia podpisu |
| kolejka/merge wyświetlania pending | poprzez `UI_ClearPending()` | przejście UX z trybu potwierdzenia |

## Zależności

Bezpośrednie zależności:
- Dane wejściowe: `g_tx_queue` (`wallet_tx_t`)
- Sygnał użytkownika: `g_user_event_group` + bity `EVENT_USER_CONFIRMED/REJECTED`
- Walidacja: `tx_request_validate()` i `tx_validate_result_str()`
- Warstwa kryptograficzna: `crypto_hash_sha256()`, `crypto_derive_btc_m44_0_0_0_0()`, `crypto_sign_btc_hash()`
- Seed: `get_wallet_seed()` (stub słaby w tym module; rzeczywista implementacja może być w `wallet_seed.c` dla kompilacji testowych)
- UI/rejestrowanie: `Task_Display_Log()`, `UI_ClearPending()` i chroniony dostęp do `g_display_ctx`

Globalne struktury/flagi które są modyfikowane:
- `g_last_sig`, `g_last_sig_ready`
- `g_security_alert`
- pola `g_display_ctx` (currency/amount/safe_locked/signature_valid)

## Relacje modułów

- `task_net.md` (tworzy/umieszcza ładunek w `g_tx_queue`)
- `tx_request_validate.md` (walidacja)
- `task_user.md` (bity zdarzeń confirm/reject)
- `task_display.md` (renderowanie pending/UX)
- `crypto_wallet.md` / `memzero.md` (kryptografia i czyszczenie)
