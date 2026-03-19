\page task_display_minimal "task_display_minimal: minimal-lwip wyświetlacz + mirroring logów"
\related Task_Display_Create
\related Task_Display_Log

# `task_display_minimal.c`

<brief>Moduł `task_display_minimal` jest lekką implementacją UI/logowania dla `minimal-lwip`: minimalizuje obciążenie SSD1306, lustrzuje wiadomości do UART i pisze krótki ogon logu do `g_display_ctx`, aby wyświetlacz można było aktualizować tylko w razie potrzeby.</brief>

## Przegląd

Moduł `task_display_minimal` jest lekką implementacją UI/logowania dla `minimal-lwip`: minimalizuje obciążenie SSD1306, lustrzuje wiadomości do UART i pisze krótki ogon logu do `g_display_ctx`, aby wyświetlacz można było aktualizować tylko w razie potrzeby.

## Przepływ logiki (minimalny OLED loop)

Przepływ zadania:
1. Na starcie, loguje komunikat startu i (jeśli `!SKIP_OLED`) wykonuje szybkie wyjście "+ LwIP / DHCP..."
2. Następnie zadanie wchodzi w nieskończoną pętlę:
   - miga LED1 jako wskaźnik "alive"
   - opcjonalnie drukuje "Disp: alive" co 5 sekund (jeśli `LWIP_ALIVE_LOG`)
   - jeśli OLED włączony (`!SKIP_OLED`), okresowo czyta `g_display_ctx` pod mutexem i aktualizuje ekran (IP + ogon logu)
   - opóźnienie `500ms` między aktualizacjami

### Scenariusze rozgałęzienia flagi

| Flaga | Efekt na wyświetlaczu |
|-------|---|
| `SKIP_OLED=1` | Ruch I2C/SSD1306 całkowicie wyłączony; zadanie pozostaje tylko z alive/logowaniem |
| `LWIP_ALIVE_LOG` | Dodaje okresowy UART+log alive, aby zobaczyć żywość sieci |

## Przerwania i rejestry

Brak bezpośrednich rejestrów/ISR. Tylko część "niskiego poziomu": bezpośrednie wywołania sterownika SSD1306 nad I2C, chronione mutexem.

## Czasy i warunki rozgałęzienia

| Operacja | Wartość |
|----------|---------|
| tworzenie/startup | `vTaskDelay(100ms)` po starcie wyświetlacza |
| I2C/ctx mutex | `xSemaphoreTake(..., 50ms)` / `xSemaphoreTake(..., 20ms)` |
| pętla aktualizacji OLED | `vTaskDelay(500ms)` |
| kolejka UI | nie używana w minimalnej kompilacji (funkcje UI merge zablokowały) |

## Zależności

- Globalna struktura kontekstu UI: `g_display_ctx` i `g_display_ctx_mutex` (dla linii logu + ciąg IP)
- Współdzielony zasób I2C: `g_i2c_mutex`
- Logowanie: `UART_Log()` (poprzez `Task_Display_Log()`)
- SSD1306: `ssd1306_*` + `Font_6x8`
- Polityka LED (pośrednio): miga LED1 jako wskaźnik żywości wyświetlacza

## Relacje modułów

- `task_display.md` (pełna wersja UI)
- `task_net.md` / `app_ethernet_cw.md` (dostarczają IP/DHCP które się wyświetlają)
- `hw_init.md` (I2C/SSD1306 init w głównej ścieżce)
