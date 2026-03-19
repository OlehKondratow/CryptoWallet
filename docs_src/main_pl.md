\page main "main: RTOS bootstrap + task wiring"
\related Error_Handler
\related HW_Init
\related HW_Init_Early_LwIP
\related Task_Display_Create
\related Task_Net_Create
\related Task_Sign_Create
\related Task_IO_Create
\related Task_User_Create

# `main.c` + `main.h`

<brief>Moduł `main` opisuje "sklejenie" całej aplikacji STM32H7: określa kolejność wczesnej inicjalizacji HAL/LwIP, tworzy obiekty IPC (kolejki, semafory, grupy zdarzeń), uruchamia zadania krytyczne FreeRTOS i uruchamia harmonogram.</brief>

## Przegląd
<brief>Moduł `main` opisuje "sklejenie" całej aplikacji STM32H7: określa kolejność wczesnej inicjalizacji HAL/LwIP, tworzy obiekty IPC (kolejki, semafory, grupy zdarzeń), uruchamia zadania krytyczne FreeRTOS i uruchamia harmonogram.</brief>

## Abstract (Synteza logiki)
`main` jest punktem wejścia, który przekształca zbiór bloków sprzętowych (zegary, urządzenia peryferyjne) i zadań aplikacji (wyświetlacz/sieć/podpis/io/użytkownik) w działający system. Cała "logika biznesowa" jest rozłożona na zadania, ale `main` ustanawia kontrakt danych: które globalne struktury służą jako kanały przesyłania zdarzeń, które mutexy chronią zasoby wspólne (I2C/UI) i które flagi startowe są pobierane przez warstwy poniżej (np. czas/pamięć podręczna dla LwIP).

### Niezmienniki krytyczne
1. Przed `HAL_Init()` uruchamia się wczesna gałąź wyrównana z LwIP (MPU/pamięć podręczna).
2. Przed utworzeniem zadań tworzą się obiekty IPC i mutexy, aby zadania nie działały z deskryptorami NULL.
3. Kolejność inicjalizacji odpowiada stylowi "odniesienia" LwIP/Cube (lwip_zero / lwip-uaid-SSD1306).

## Logic Flow (sekwencja bootstrap)
Nie ma liniowej maszyny stanu na poziomie "stanów systemu", ale istnieje sekwencja faz:

1. Przygotowanie sprzętu (obejście wyjątku niewyrównanego dostępu dla M7).
2. Pakiet wczesnej konfiguracji LwIP (tylko jeśli `USE_LWIP`).
3. Standardowa inicjalizacja HAL, a następnie `HW_Init()` (GPIO/I2C/UART/USB/RNG).
4. Inicjalizacja usługi czasu.
5. Warunkowe włączenie crypto-RNG.
6. W "scenariuszu produkcyjnym" (nie `BOOT_TEST`): tworzenie kolejek/semaforów/grupy zdarzeń i uruchomienie rdzenia.
7. Tworzenie zadań w ustalonej kolejności (wyświetlacz → sieć → podpis → io → użytkownik).
8. Uruchomienie harmonogramu.

## Przerwania/rejestry
Bezpośrednia manipulacja rejestrem urządzenia peryferyjnego w `main.c` jest minimalna; główny krok "niskiego poziomu" to zmiana rejestru zachowania systemu:

| Działanie | Powód |
|---|---|
| `SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk` | Zezwól na niewyrównany dostęp, gdzie stos/struktury LwIP mogą czytać pola protokołów niewyrównane. |

## Timed i warunkowe gałęzie

| Warunek | Wynik |
|---|---|
| `BOOT_TEST` | Zamiast FreeRTOS uruchom pętlę diagnostyczną z `HAL_Delay(500)`. |
| `SKIP_OLED` | Baner na SSD1306 nie jest rysowany (SSD1306 już zainicjowany w `HW_Init()`). |
| `USE_CRYPTO_SIGN` | Podnieś `crypto_rng_init()` (jeśli kryptografia włączona). |

## Zależności
Bezpośrednie relacje danych/wywołań:

- **Sprzęt:** `HW_Init_Early_LwIP()`, `HW_Init()`, `HAL_Init()`.
- **Czas:** `time_service_init()`.
- **IPC & kontrakt zadań:** `g_tx_queue`, `g_display_queue`, `g_user_event_group`, `g_i2c_mutex`, `g_ui_mutex`, `g_display_ctx_mutex`, `g_display_ctx`.
- **Zadania:** `Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`.
- **UI/logowanie:** `Task_Display_Log()` i pośrednio `UART_Log()` (przez warstwę wyświetlacza).

Globalne struktury/flagi tworzone/inicjalizowane w `main`:

- **Kolejki:** `g_tx_queue` (sieć → podpis), `g_display_queue` (migawka tx → wyświetlacz).
- **Grupa zdarzeń:** `g_user_event_group` (użytkownik → podpis potwierdź/odrzuć).
- **Mutexy:** chroń I2C/UI/kontekst wyświetlacza.
- **Flagi/status:** `g_security_alert` i `g_last_sig_ready` pozostają na domyślnych (zarządzane później przez zadania).

## Relacje
- Warstwa sprzętu: `hw_init.md`
- Wyświetlacz/logowanie: `task_display.md`, `task_display_minimal.md`
- Sieć: `task_net.md`, `app_ethernet_cw.md`
- Potwierdzenie/bezpieczeństwo: `task_user.md`, `task_sign.md`, `task_security.md`
- Czas: `time_service.md`
