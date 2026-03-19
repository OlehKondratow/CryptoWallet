\page time_service "time_service: SNTP -> epoch + formatowanie UTC"
\related time_service_init
\related time_service_start
\related time_service_now_epoch

# `time_service.c` + `time_service.h`

<brief>Moduł `time_service` zapewnia synchronizację czasu poprzez SNTP i daje aplikacji zunifikowany dostęp do bieżącej epoki Uniksa i reprezentacji ciągu UTC (dla dzienników/UI), zbudowanej na bazie `HAL_GetTick()` po otrzymaniu epoki z sieci.</brief>

## Przegląd

Moduł `time_service` zapewnia synchronizację czasu poprzez SNTP i daje aplikacji zunifikowany dostęp do bieżącej epoki Uniksa i reprezentacji ciągu UTC dla dzienników/UI, zbudowanej na bazie `HAL_GetTick()` po otrzymaniu epoki z sieci.

## Przepływ logiki (SNTP lifecycle)

Cykl życiowy:
1. Inicjalizacja: reset flag i wartości bazowych; napraw bieżący `HAL_GetTick()` jako kotwicę startową
2. Start klienta SNTP: moduł działa tylko raz (guard `s_sntp_started`), inicjalizacja poprzez `tcpip_callback` aby kod żył w kontekście LwIP
3. Otrzymaj epoch: na synchronizacji, callback przechowuje `s_epoch_base` i `s_tick_base_ms`, ustawia `s_time_synced`
4. Każdy raz:
   - oblicz "now epoch" jako `epoch_base + (HAL_GetTick()-tick_base)/1000`
   - sformatuj ciąg UTC (jeśli `s_time_synced`, inaczej drukuj `UNSYNCED`)

### Algorytm Epoch → UTC

Konwersja bez RTC: podziel przez dni/sekundy i sekwencyjne odejmowanie dni po latach/miesiącach, biorąc pod uwagę lata przestępne.

| Krok | Jak obliczony |
|------|---|
| Rozkład | epoch_sec → dni + reszta → godzina/minuta/sekunda |
| Lata | odejmij `365/366` podczas gdy dni wciąż w bieżącym roku |
| Miesiące | odejmij dni po tabeli miesięcy, korygując luty na lata przestępne |

## Przerwania i rejestry

Brak bezpośrednich rejestrów. Ale jest asynchronia:
- start SNTP inicjowany poprzez `tcpip_callback` (callback oczekiwany do uruchomienia w kontekście wątku TCP/IP LwIP)
- obliczanie "now epoch" powiązane z `HAL_GetTick()`, tj. z systemowym zegarem/timerem HAL

## Czasy i warunki rozgałęzienia

| Guard/Flaga | Warunek | Zachowanie |
|---|---|---|
| `s_sntp_started` | powtórzony wyw start | start zignorowany |
| `s_time_synced` | żądanie ciągu/czasu | `time_service_now_string` zwraca `UNSYNCED`, obliczanie epoki bazuje na ostatnim epoch_base |

Punkt krytyczny:
- `time_service_start()` zwraca "sukces" tylko na fakcie umieszczenia callback w `tcpip_callback`; wynik sieci przychodzi później

## Zależności

Bezpośrednie:
- LwIP: `lwip/apps/sntp.h`, `lwip/tcpip.h` (SNTP init + uruchomienie w wątku TCP/IP)
- Czas/timebase: `HAL_GetTick()`
- UI/rejestrowanie: `Task_Display_Log()` na start/błędy/synchronizacja

Globalne struktury/flagi:
- lokalne statyczne: `s_sntp_started`, `s_time_synced`, `s_epoch_base`, `s_tick_base_ms`

## Relacje modułów

- `main.md`: uruchamia init usługi i inicjuje bootstrap
- `task_net.md` / `app_ethernet_cw.md`: zwykle wyzwalają moment "po linku" (kiedy wezwać `time_service_start()`)
- `task_display.md`: pokazuje ciągi czasu/ogony dzienników
