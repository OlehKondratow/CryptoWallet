\page task_display "task_display: SSD1306 UI (4-liniowy scroll) + UI merge"
\related Task_Display_Create
\related UI_UpdateData
\related Task_Display_Log
\related UI_ClearPending

# `task_display.c` + `task_display.h`

<brief>Moduł `task_display` zarządza stanem wizualnym portfela na SSD1306: odbiera zdarzenia sieciowe/podpisywania, łączy je w jeden wyświetlany stan i renderuje 4 linie UI z "przewijającym się tekstem" dla logów i danych sieciowych.</brief>

## Przegląd

Moduł `task_display` zarządza stanem wizualnym portfela na SSD1306: odbiera zdarzenia sieciowe/podpisywania, łączy je w jeden wyświetlany stan i renderuje 4 linie UI z "przewijającym się tekstem" dla logów i danych sieciowych.

## Przepływ logiki (UI rendering + queue/event merge)

Główna pętla zadania wyświetlacza:
1. Inicjalizacja buforów wewnętrznych
2. Oczekiwanie: zadanie próbuje wyodrębnić "snapshot" transakcji oczekującej z kolejki z timeoutem
3. Aktualizacja wewnętrznego kontekstu UI (flaga oczekiwania + dane transakcji) z ochroną mutex
4. Renderowanie bieżących 4 linii na SSD1306
5. Okresowe opóźnienie dla "szybkości tikowania" aktualizacji/przewijania

### Stany UI (Jak "żyje")

Brak jawnego enum state machine jak "klasyczna" automatyka, ale warunkowy układ na 4 liniach:

| Linia | Co wyświetla | Ruch/przewijanie |
|-------|---|---|
| 0 | WALLET: moneta, kwota, skrócony odbiorca + wskaźnik trybu `Confirm?` | statyczne (brak scroll) |
| 1 | SECURITY: Safe locked/unlocked + podpowiedź/status podpisu | statyczne |
| 2 | NETWORK: IP + MAC | scroll (jeśli linia dłuższa) |
| 3 | LOG: ogon ostatnich wiadomości | scroll |

Offset przewijania zaimplementowany via `s_line_offset[row]`:
- linie 0-1: offset wymuszony na 0
- linie 2-3: offset inkrementowany modulo długość linii

## Przerwania i rejestry

Moduł to zadanie FreeRTOS, brak bezpośredniej pracy z rejestrami. Tylko "podobna do rejestru" część: dostęp do sterownika UI SSD1306, który ukrywa pracę niskiego poziomu poprzez I2C.

Krytyczne blokady:

| Zasób | Jak chroniony | Gdzie |
|-------|---|---|
| I2C | weź `g_i2c_mutex` (w render_lock/unlock) | wypełnianie/wyjście do SSD1306 |
| Wspólne dane UI | `g_ui_mutex` | łączenie oczekiwania/kontekstów |
| display context | `g_display_ctx_mutex` | aktualizowanie ogona logu |

## Czasy i gałęzie

| Parametr | Wartość | Znaczenie |
|----------|---------|-----------|
| kolejka wyświetlacza | timeout `QUEUE_WAIT_MS=100ms` | aktualizacje oczekiwania nie blokują renderowania długo |
| log scroll | `LOG_SCROLL_MS=120ms` | prędkość "tick rate" wyświetlacza |
| blokada I2C | `portMAX_DELAY` | zapewnia atomowe wyjście do OLED |
| mutex UI | wait `pdMS_TO_TICKS(50)` / render `pdMS_TO_TICKS(20)` | zapobiega zawieszeniu, ogranicza max opóźnienia |

Gałęzie:
- jeśli kolejka oczekiwania nie podała danych — UI pozostaje w bieżącym stanie i po prostu kontynuuje renderowanie linii
- dynamika `Confirm?` zależy od `pending_tx.is_pending`

## Zależności

Bezpośrednie zależności:
- Kanały globalne: `g_display_queue`, `g_ui_mutex`, `g_i2c_mutex`, `g_display_ctx_mutex`, wewnętrzne struktury `s_ui_data`
- Wspólne typy UI: `Transaction_Data_t`, `UI_Display_Data_t` (z `task_display.h`)
- Sterownik wyświetlacza: `ssd1306_*` i `Font_6x8`

Pośrednio wpływa/koordynuje się z:
- sieć/HTTP: poprzez umieszczenie `pending_tx` w kolejce (w `task_net.c` / logika żądania)
- podpisywanie: gdy po potwierdzeniu/odrzuceniu oczekiwanie przechodzi z trybu potwierdzenia (poprzez `UI_ClearPending()`)

## Relacje modułów

- `task_display_minimal.md` (lekka wersja dla minimal-lwip)
- `task_net.md` (generacja snapshot oczekiwania)
- `task_sign.md` (clear pending i zmiana UI security/lock)
- `task_user.md` (potwierdzenie/odrzucenie poprzez grupę zdarzeń)
- `hw_init.md` (init I2C/SSD1306 base)
