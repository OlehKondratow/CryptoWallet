\page wallet_shared "wallet_shared: kontrakty IPC/danych dla net/display/sign/user"
\related g_tx_queue
\related g_display_queue
\related g_user_event_group
\related g_i2c_mutex
\related g_ui_mutex
\related g_display_ctx_mutex

# `wallet_shared.h` (kontrakt danych + IPC)

<brief>Nagłówek `wallet_shared` definiuje zunifikowany kontrakt między modułami: struktury danych dla "żądanie → potwierdzenie → podpis" i stan UI, jak również globalne uchwyty IPC (kolejki, grupy zdarzeń, mutexy) które są wymieniane między `task_net`, `task_sign`, `task_user` i `task_display`.</brief>

## Przegląd

Nagłówek `wallet_shared` definiuje zunifikowany kontrakt między modułami: struktury danych dla "żądanie → potwierdzenie → podpis" i stan UI, jak również globalne uchwyty IPC (kolejki, grupy zdarzeń, mutexy) które są wymieniane między `task_net`, `task_sign`, `task_user` i `task_display`.

## Przepływ logiki

W architekturze systemów osadzonych "logika biznesowa" jest rozproszona między zadaniami, a `wallet_shared.h` jest warstwą umów: dokładnie jak moduł sieciowy zmienia się w obiekt zrozumiały dla modułu podpisywania, jak przycisk UX zmienia się w zdarzenie dyskretne, i jak status bezpiecznego/podpisanego portfela zmienia się w to co rysuje SSD1306. Bez tego kontraktu, każdy podział zadań między wątkach zmienia się w chaotyczne przekazywanie danych.

## Model formalny

Brak jawnej maszyny stanów w nagłówku, ale istnieje "formalny" model:
1. Sieć/host tworzy `wallet_tx_t` (recipient/amount/currency)
2. `task_sign` konsumuje `wallet_tx_t` z `g_tx_queue` i przechodzi UI poprzez `g_display_ctx`
3. `task_user` ustawia zdarzenia w `g_user_event_group`, które `task_sign` czeka
4. UI otrzymuje aktualizacje albo poprzez `g_display_queue` albo poprzez `g_display_ctx`

## Przerwania i rejestry

Nagłówek tylko deklaruje typy/globalne deskryptory. Nie zawiera rejestrów/ISR.

## Zależności

Kluczowe zależności danych:
- Typy i rozmiary ciągów zgodzony między walidacją/renderowaniem:
  - `TX_RECIPIENT_LEN`, `TX_AMOUNT_LEN`, `TX_CURRENCY_LEN`
- Zdarzenia użytkownika:
  - `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (przesłane do `task_sign`)
- Stany UI/kontekst:
  - `display_state_t` (zestaw ekranów) i `display_context_t` (dane do renderowania)
- Stany FSM podpisywania:
  - `signing_state_t` jako wspólna enum (zarówno dla aktywnego task_sign jak i legacy task_security)

Globalne uchwyty (tworzone w `main.c`, używane w zadaniach):

| Uchwyt | Typ | Zwykle pisze | Czyta/czeka |
|--------|-----|---|---|
| `g_tx_queue` | `QueueHandle_t` | `task_net` | `task_sign` |
| `g_display_queue` | `QueueHandle_t` | `task_net` | `task_display` |
| `g_user_event_group` | `EventGroupHandle_t` | `task_user` | `task_sign` |
| `g_i2c_mutex` | `SemaphoreHandle_t` | display | display-only krytyczna sekcja |
| `g_ui_mutex` | `SemaphoreHandle_t` | task_display | merge kontekstu |
| `g_display_ctx_mutex` | `SemaphoreHandle_t` | net/sign | czytaj w wyświetlaczu |

Globalne zmienne stanu/wyniku:
- `g_security_alert` — wskaźnik mapowany do LED3
- `g_last_sig[64]` i `g_last_sig_ready` — ostatni podpis, odpytywany przez warstwę sieciową/USB

## Relacje modułów

- Konsument podpisu: `task_sign.md`
- Konsument zdarzeń UX: `task_user.md`
- Walidacja payloadu hosta: `tx_request_validate.md`
- Wyświetlacz UI: `task_display.md`
- Polityka wyświetlania alertu: `task_io.md`
- Montaż payloadu sieciowego/kolejki: `task_net.md`, `app_ethernet_cw.md`
