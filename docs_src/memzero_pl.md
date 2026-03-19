\page memzero "memzero: secure buffer zeroing"
\related memzero

# `memzero.c` + `memzero.h`

<brief>Moduł `memzero` bezpiecznie czyści wrażliwe bufory (klucze prywatne, skróty, nasiona) poprzez zapisywanie volatile, zapobiegając optymalizacji kompilatora i eliminując ślady niebezpiecznych danych z pamięci.</brief>

## Przegląd

<brief>Moduł `memzero` bezpiecznie czyści wrażliwe bufory (klucze prywatne, skróty, nasiona) poprzez zapisywanie volatile, zapobiegając optymalizacji kompilatora i eliminując ślady niebezpiecznych danych z pamięci.</brief>

## Abstrakcja (synteza logiki)

`memzero` rozwiązuje jeden krytyczny problem bezpieczeństwa: nowoczesne kompilatory mogą zoptymalizować "bezużyteczne" `memset(buf, 0, len)` jeśli kompilator zauważy, że `buf` nie jest już używany po tym. Podczas pracy z kluczami prywatnymi jest to niebezpieczne — wrażliwe dane pozostają w pamięci i mogą zostać wydobyte podczas ataku. Rozwiązanie: użyć wskaźnika `volatile`, którego kompilator nie może zoptymalizować. Rola biznesowa: gwarantować, że na każdej ścieżce wyjścia (sukces lub błąd) dane prywatne są natychmiast nadpisane zerami przed zwrotem funkcji.

## Przepływ logiki

Funkcja `memzero(void *pnt, size_t len)`:

1. Konwertuje wskaźnik na `volatile unsigned char *`
2. Pętla: wierszowe pisanie zer poprzez volatile wskaźnik
3. Zwrot

Brak automatu stanów, brak logiki warunkowej — po prostu niezawodne nadpisywanie.

## Przerwania/rejestry

Brak ISR lub rejestrów: operacja zapisywania w pamięci na poziomie C.

## Czasy i warunki rozgałęzienia

| Parametr | Zachowanie |
|---|---|
| Wskaźnik NULL | no-op (len-- natychmiast wychodzi z pętli) |
| len = 0 | no-op (brak iteracji) |
| Dowolny rozmiar buforu | jeden bajt na iterację, poprzez zapis volatile |

## Zależności

Brak zależności: tylko `<stddef.h>` dla `size_t`.

Używane z:
- `task_sign.c` — czyści seed/skrót/podpis po błędach lub pomyślnym podpisaniu
- `task_security.c` — to samo w legacy FSM
- `crypto_wallet.c` — czyści HDNode i klucz prywatny
- `wallet_seed.c` — czyści bufor nasiona

## Relacje

- `task_sign.md` (główny konsument)
- `crypto_wallet.md` (bezpieczeństwo przy podpisywaniu)
- `wallet_seed.md` (obsługa nasiona)
