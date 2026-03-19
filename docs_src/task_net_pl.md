\page task_net "task_net: LwIP + serwer HTTP dla API podpisywania transakcji"
\related Task_Net_Create
\related http_server_thread
\related parse_tx_json
\related parse_tx_form

# `task_net.c` + `task_net.h`

<brief>Moduł `task_net` jest fasadą sieciową aplikacji: podnosi LwIP/Ethernet, uruchamia serwer HTTP na porcie 80, parsuje żądania POST JSON/form (`POST /tx`), waliduje transakcje i wysyła je do podpisu poprzez `g_tx_queue`.</brief>

## Przegląd

Moduł `task_net` jest punktem wejścia dla wszystkich zewnętrznych żądań podpisu: host (PC, mobile, interfejs webowy) łączy się poprzez Ethernet, wysyła JSON lub formularz z adresem/kwotą/walutą, a moduł parsuje, waliduje, wyświetla na SSD1306 i umieszcza w kolejce dla zadania podpisywania. Bez LwIP (`USE_LWIP=0`), moduł jest no-op. Rola biznesowa: być "bramą do mikrokontrolera" dla klienta sieciowego.

## Przepływ logiki (HTTP server FSM)

1. **Inicjalizacja:**
   - `Task_Net_Create()` z `main.c` tworzy zadanie FreeRTOS z priorytetem `NET_PRIORITY`
   - Zadanie `net_task()` inicjalizuje `tcpip_init()` (jądro LwIP)
   - Konfiguruje netif: DHCP (jeśli `LWIP_DHCP=1`) lub statyczny IP (z `main.h`)
   - Ustawia callback na `ethernet_link_status_updated()`
   - Startuje oddzielny wątek `http_server_thread()` ze stosem 16K

2. **HTTP Server FSM:**
   - Tworzy słuchacz TCP na porcie 80
   - Pętla: `netconn_accept()` czeka na połączenie przychodzące
   - Otrzymuje wszystkie dane w jednym wywołaniu `netconn_recv()` (timeout 2 sek)

3. **Request parsing:**
   - Parsuje pierwszą linię HTTP: `GET /path` lub `POST /path`
   - Dla `GET /ping`: zwraca "pong"
   - Dla `GET /`: zwraca formularz HTML dla interfejsu
   - Dla `GET /tx/signed`: zwraca JSON z ostatnią sygnaturą (`g_last_sig` + flaga `g_last_sig_ready`)
   - Dla `POST /tx`:
     - Parsuje body jako JSON (szuka `"recipient"`, `"amount"`, `"currency"`) lub formularz (`recipient=...&amount=...&currency=...`)
     - Waliduje poprzez `tx_request_validate()`
     - Jeśli ważny, wysyła do `g_tx_queue` do podpisu i aktualizuje wyświetlacz poprzez `g_display_queue`

4. **Odpowiedzi:**
   - HTTP 200 z HTML/JSON/text
   - HTTP 404 dla nieznanych ścieżek

## Przerwania i rejestry

`task_net` nie dotyka bezpośrednio ISR/rejestrów. Używa API LwIP (netconn, netbuf). Interakcja z HAL tylko poprzez `ethernet_link_status_updated()` (z `app_ethernet_cw.c`).

## Czasy i warunki rozgałęzienia

| Endpoint | Metoda | Zachowanie |
|----------|--------|-----------|
| `/ping` | GET | Natychmiast zwróć "pong" |
| `/` | GET | Wyślij formularz HTML |
| `/tx/signed` | GET | Wyślij JSON z ostatnią sygnaturą lub statusem |
| `/tx` | POST | Parsuj body (JSON lub formularz), waliduj, wyślij do `g_tx_queue` |
| Inne | GET/POST | HTTP 404 |

Timing:
- HTTP request timeout: 2 sek (netconn_set_recvtimeout)
- Queue send timeout: 100 ms (pdMS_TO_TICKS(100))
- Task main loop: 2 sek opóźnienie do aktualizacji wyświetlacza (IP po DHCP)

## Zależności

Bezpośrednie zależności:
- **FreeRTOS:** xTaskCreate, xQueueSend, vTaskDelay, xSemaphoreTake/Give
- **LwIP:** netconn API, tcpip_init, netif
- **wallet_shared.h:** `wallet_tx_t`, `g_tx_queue`
- **tx_request_validate.h:** walidacja przed kolejką
- **task_display.h:** `Task_Display_Log()`, `g_display_queue`, `g_display_ctx_mutex`
- **app_ethernet.h:** callback `ethernet_link_status_updated()`
- Globalne z innych modułów: `g_last_sig[]`, `g_last_sig_ready` (z `task_sign`)

## Relacje modułów

- `wallet_shared.md` (struktura wallet_tx_t i kolejka)
- `tx_request_validate.md` (walidacja przed umieszczeniem w kolejce)
- `task_sign.md` (konsument z g_tx_queue)
- `task_display.md` (aktualizacje wyświetlacza)
- `app_ethernet_cw.md` (callbacki linku i informacja zwrotna LED)
