\page task_security "task_security: legacy/mock signing FSM"
\related Task_Security_Create

# `task_security.c` + `task_security.h`

<brief>Moduł `task_security` przechowuje wariant "legacy/mock" FSM podpisywania do bring-up i porównania: nie jest używany w głównej ścieżce (`main.c` go nie wywołuje), ale implementuje podobną automatykę potwierdzenia i zastępuje kryptografię atrapami.</brief>

## Przegląd

Moduł `task_security` przechowuje wariant "legacy/mock" FSM podpisywania do bring-up i porównania: nie jest używany w głównej ścieżce (`main.c` go nie wywołuje), ale implementuje podobną automatykę potwierdzenia i zastępuje kryptografię atrapami.

## Przepływ logiki (Legacy FSM)

Wewnętrzna maszyna stanów jest zaimplementowana jako:
1. Zmienna lokalna `state` zaczyna się w `SIGNING_IDLE`
2. W trybie IDLE, zadanie pobiera ładunek z `g_tx_queue` (timeout 200ms) i przechodzi do `SIGNING_RECEIVED`
3. Następnie na każde tictac, jedna iteracja `fsm_signing_step` jest wykonywana, która przetwarza:
   - RECEIVED: wykonuje mock-hash i przechodzi do WAIT_CONFIRM
   - WAIT_CONFIRM: czeka na bity zdarzeń confirm/reject z timeoutem 30s
   - IN_PROGRESS: wykonuje mock-sign i przechodzi do DONE (lub ERROR)
4. Gdy osiągnięto stany terminalne (DONE/REJECTED/ERROR), zadanie czyści tx i powraca do IDLE

### Aktualizacja stanu wyświetlacza

Moduł zawiera helper "update signing on display context", który mapuje stan FSM na pola `g_display_ctx` (locked/valid/pending).

## Przerwania i rejestry

Brak dostępu ISR/rejestru. Moduł używa:
- oczekiwania grupy zdarzeń
- ładunku kolejki
- czyszczenia pamięci `memzero()` dla wrażliwych buforów na ścieżkach wyjścia

## Czasy i gałęzie

| Parametr | Wartość |
|----------|---------|
| timeout na odbiór kolejki | 200ms |
| timeout confirm/reject | 30000ms |
| przetwarzanie | per iteracje pętli (brak ścisłego okresowego sleep, tylko opóźnienia z kolejek/oczekiwań) |

Kluczowe rozgałęzienie:
- reject/timeout → SIGNING_REJECTED (lub powrót poprzez stan terminalny)
- brak reject → CONFIRM → SIGNING_IN_PROGRESS → SIGNING_DONE

## Zależności

Bezpośrednie zależności:
- `g_tx_queue` / `wallet_tx_t` (input)
- `g_user_event_group` / `EVENT_USER_CONFIRMED/REJECTED` (sygnał UX)
- `g_display_ctx_mutex` i `g_display_ctx` (wyjście statusu)
- `memzero()` (sanityzacja)
- Funkcje mock-crypto wewnątrz modułu (nie rzeczywiste peryferia krypto)

Osobliwość architektoniczna:
- W `main.c`, `Task_Security_Create()` nie jest wywoływany; więc moduł domyślnie nie wpływa na ścieżkę produkcji

## Relacje modułów

- `task_sign.md` (production FSM, rzeczywista kryptografia)
- `task_user.md` (źródło bitów zdarzeń)
- `wallet_shared.md` (wspólny kontrakt stanu FSM + bity zdarzeń)
- `task_display.md` (warstwa wyświetlacza kontekstu/logowania)
