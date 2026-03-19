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

<brief>Moduł `main` jest orkiestratorem systemu: inicjalizuje sprzęt (HAL, wczesna inicjalizacja LwIP, taktowanie), ustanawia kontrakty IPC (kolejki, semafory, grupy zdarzeń, stan globalny), inicjalizuje usługi czasu/kryptografii oraz uruchamia podstawową rodzinę zadań FreeRTOS (wyświetlacz, sieć, podpis, IO, wejście użytkownika). Punkt wejścia całej aplikacji portfela osadzonego.</brief>

## Przegląd

`main.c` to "kod pośredni" który przekształca niezależne bloki sprzętu i moduły aplikacji w pracujący system. To **nie** automat stanów i **nie** ciągły proces roboczy — to **jednorazowa sekwencja rozruchu**, która:

1. Inicjalizuje sprzęt w prawidłowej kolejności (krytyczne dla LwIP/DMA/spójności pamięci podręcznej)
2. Tworzy prymitywy międzyprocesowej komunikacji (IPC), od których zależy każde zadanie
3. Instancjonuje rodzinę zadań i uruchamia harmonogram FreeRTOS
4. Nigdy nie wraca (harmonogram działa w nieskończoność)

## Abstrakcja (synteza logiki)

**Problem:** portfel osadzony musi:
- Akceptować żądania podpisu z sieci (HTTP) lub interfejsów USB
- Pozwolić użytkownikowi potwierdzić/odrzucić za pośrednictwem przycisku
- Wyświetlać status na OLED 128×32
- Wykonywać podpis ECDSA jeśli kryptografia jest włączona
- Bezpiecznie zarządzać wieloma równoczesnymi zadaniami (bez race condition)

**Rozwiązanie:** `main.c` zapewnia **kontrakt architektoniczny**:

1. **Kolejność inicjalizacji sprzętu ma znaczenie:** LwIP wymaga, aby MPU/pamięć podręczna były skonfigurowane **przed** `HAL_Init()`. To nie jest oczywiste, ale jest replikowane z przykładu `lwip_zero` w STM32CubeH7.

2. **Stan globalny jest zamierzony:** wszystkie zadania dzielą:
   - `g_tx_queue`: żądania transakcji z warstwy sieciowej do warstwy podpisywania
   - `g_display_queue`: dane transakcji (do wyświetlacza) z sieci
   - `g_user_event_group`: zdarzenia przycisku sygnalizują potwierdzenie/odrzucenie
   - `g_i2c_mutex`, `g_ui_mutex`, `g_display_ctx_mutex`: serializują dostęp do magistrali I2C (OLED) i stanu UI

3. **Kolejność tworzenia zadań jest deterministyczna:** zadanie wyświetlacza jest tworzone najpierw (priorytet logowania), potem sieć, potem podpis, potem wskaźniki i wejście użytkownika. Zapewnia to wczesny wyjście diagnostyczne.

4. **Haki łapią katastrofalne awarie:**
   - `vApplicationMallocFailedHook()`: brak dostępnej pamięci
   - `vApplicationStackOverflowHook()`: przepełnienie stosu zadania
   - `Error_Handler()`: ostateczny punkt wyjścia, który loguje i zatrzymuje

## Przepływ logiki (sekwencja rozruchu)

### Faza 0: przygotowanie przed harmonogramem (blokujące)

```
┌─────────────────────────────────────────────────┐
│ SCB->CCR: wyłączyć trap niewyrównanego dostępu  │
│ (stosy LwIP/protokołów mogą czytać pola         │
│  z niewyrównanym dostępem)                      │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HW_Init_Early_LwIP() [jeśli USE_LWIP]           │
│  • Konfiguracja regionu MPU (pamięć, ochrona)   │
│  • Włączenie pamięci podręcznej Cortex-M7       │
│  • Inicjalizacja puli pamięci LwIP              │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HAL_Init()                                      │
│  • SystemClock_Config() (konfiguracja PLL STM32)│
│  • Włączenie taktu HAL przez SysTick            │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ HW_Init()                                       │
│  • Inicjalizacja GPIO (LED, przycisk)           │
│  • I2C1 dla SSD1306                             │
│  • UART3 do logowania                           │
│  • Urządzenie USB (WebUSB)                      │
│  • RNG (entropia) jeśli USE_CRYPTO_SIGN         │
│  • Inicjalizacja wyświetlacza SSD1306           │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ time_service_init()                             │
│  • Konfiguracja SNTP do synchronizacji czasu    │
│  • Inicjalizacja RTC lub czasu opartego na tiku │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
   ┌───────────────────────────────────┐
   │ jeśli (USE_CRYPTO_SIGN)           │
   │   crypto_rng_init()               │
   │   (pula entropii trezor-crypto)   │
   └───────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ jeśli (!BOOT_TEST):                             │
│  ├─ Utworzyć prymitywy IPC                      │
│  │  • g_tx_queue (4 × wallet_tx_t)              │
│  │  • g_display_queue (4 × Transaction_Data_t) │
│  │  • g_user_event_group                        │
│  │  • g_i2c_mutex, g_ui_mutex, display_mutex   │
│  │                                              │
│  ├─ Walidować tworzenie (sprawdzenie NULL)      │
│  │  Error_Handler() jeśli coś się nie powiodło  │
│  │                                              │
│  └─ osKernelInitialize()                        │
│     (jądro FreeRTOS gotowe, jeszcze bez zadań)  │
│                                                 │
│  inaczej (BOOT_TEST):                           │
│  └─ Pętla: przełączać LED1, wyświetlać "boot"   │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ Tworzenie zadań (w kolejności):                 │
│  1. Task_Display_Create()  → priorytet logowania│
│  2. Task_Net_Create()      → HTTP, Ethernet     │
│  3. Task_Sign_Create()     → FSM dla ECDSA      │
│  4. Task_IO_Create()       → zarządzanie LED    │
│  5. Task_User_Create()     → obsługa przycisku  │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ Wyświetlić baner startowy na SSD1306            │
│ (jeśli !SKIP_OLED):                             │
│  "CryptoWallet + LwIP Init..."                  │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────┐
│ osKernelStart()                                 │
│ (harmonogram przejmuje kontrolę, nigdy nie      │
│  wraca)                                         │
│ (Error_Handler() jeśli osKernelStart się nie    │
│  powiedzie)                                     │
└─────────────────────────────────────────────────┘
```

### Faza 1: wykonanie zadań (ciągłe)

Gdy `osKernelStart()` jest wywoływane, harmonogram uruchamia zadania z wywłaszczeniem:

- **Task_Display**: czyta zdarzenia, aktualizuje SSD1306 ~1 Hz
- **Task_Net**: słucha Ethernet/HTTP, umieszcza transakcje w kolejce
- **Task_Sign**: czeka na transakcje → wyświetla na wyświetlaczu → czeka na przycisk → podpisuje
- **Task_IO**: miga LED1 (żywy), ustawia LED2 (status sieci), LED3 (alert)
- **Task_User**: debouncuje przycisk → wysyła potwierdzenie/odrzucenie do podpisu

Cały wspólny dostęp do danych jest chroniony przez semafory; kolejki i grupy zdarzeń zapewniają bezpieczne przekazywanie między wątkami.

## Rejestry i urządzenia peryferyjne

### Blok sterowania systemem (SCB) — Cortex-M7

| Rejestr/pole | Operacja | Przyczyna |
|---|---|---|
| `SCB->CCR` | Wyczyścić `SCB_CCR_UNALIGN_TRP_Msk` | Stos LwIP może uzyskiwać dostęp do niewyrównanych pól w nagłówkach protokołów (TCP/IP). Bez tego M7 wyrzuciłby twardy błąd. |

**Dlaczego?** Procesor Cortex-M7 może wyrzucić błąd przy niewyrównanych operacjach load/store. Ale LwIP używa struktur C, które mogą nie być wyrównane do słowa w pamięci. Inne implementacje (newlib, lwip_zero) również wyłączają tę pułapkę.

### Kolejność inicjalizacji urządzeń peryferyjnych

| Urządzenie | Moduł | Czas | Przyczyna |
|---|---|---|---|
| MPU | `HW_Init_Early_LwIP()` | Przed HAL_Init | Spójność pamięci podręcznej/DMA dla buforów LwIP |
| Taktowanie | `HAL_Init()` | Wcześnie | Konfiguracja PLL, takty przerwań |
| GPIO | `HW_Init()` | Po HAL | LED, przycisk, konfiguracja AF |
| I2C1 | `HW_Init()` | Po GPIO | Inicjalizacja SSD1306 (zależy od GPIO AF) |
| UART3 | `HW_Init()` | Po GPIO | Backend logowania |
| USB | `HW_Init()` | Po GPIO | Inicjalizacja urządzenia WebUSB |
| RNG | `HW_Init()` + `crypto_rng_init()` | Po takcie | Wysiewanie entropii |
| SSD1306 | `HW_Init()` → `ssd1306.c` | Po I2C | Wyświetlacz gotowy przed zadaniami |

## Czasy i warunki rozgałęzienia

### Flagi kompilacji

| Flaga | Typ | Zachowanie |
|---|---|---|
| `USE_LWIP` | Czas kompilacji | Włącza wызов `HW_Init_Early_LwIP()`. Bez niego konfiguracja MPU/pamięci podręcznej pominięta. |
| `BOOT_TEST` | Czas kompilacji | **Tryb diagnostyczny:** pomija FreeRTOS, pętla z `HAL_Delay(500)` i przełączaniem LED. Przydatne do testowania HAL bez harmonogramu zadań. |
| `SKIP_OLED` | Czas kompilacji (domyślnie 0) | Jeśli ustawione na 1, pomija inicjalizację SSD1306 i wyświetlanie baneru. |
| `USE_CRYPTO_SIGN` | Czas kompilacji | Jeśli ustawione, wywoła `crypto_rng_init()` do wysiewania puli entropii trezor-crypto. |

### Warunki błędów

| Warunek | Obsługa | Wynik |
|---|---|---|
| Błąd tworzenia kolejki/semafora (sprawdzenie NULL) | `Error_Handler()` | Loguje `[ERR]`, pętla nieskończona; system zatrzymany. |
| Sterta FreeRTOS wyczerpana | `vApplicationMallocFailedHook()` | Loguje `[MALLOC FAIL]`, pętla nieskończona. |
| Wykryto przepełnienie stosu zadania | `vApplicationStackOverflowHook()` | Loguje `[STACK OVF nazwa_zadania]`, pętla nieskończona. |
| `osKernelStart()` się nie powiedzie | `Error_Handler()` | Nie powinno się zdarzyć; wskazuje błąd konfiguracji FreeRTOS. |

## Stan globalny inicjalizowany w `main()`

### Kolejki

```c
g_tx_queue = xQueueCreate(4U, sizeof(wallet_tx_t));
```
- **Rozmiar:** 4 elementy
- **Element:** `wallet_tx_t` (żądanie transakcji: odbiorca, kwota, waluta)
- **Producent:** `task_net.c` (handler HTTP POST `/tx`)
- **Konsument:** `task_sign.c` (FSM podpisywania)
- **Cel:** rozdzielenie wątku sieciowego od FSM podpisywania

```c
g_display_queue = xQueueCreate(4U, sizeof(Transaction_Data_t));
```
- **Rozmiar:** 4 elementy
- **Element:** `Transaction_Data_t` (podzbiór tx dla wyświetlacza)
- **Producent:** `task_net.c` (kopia z żądania przychodzącego)
- **Konsument:** `task_display.c` (renderuje na SSD1306)
- **Cel:** wyświetlacz widzi transakcję natychmiast; FSM podpisywania może wciąż przetwarzać

### Grupa zdarzeń

```c
g_user_event_group = xEventGroupCreate();
```
- **Używane bity:**
  - `USER_CONFIRM_BIT` (bit 0): użytkownik nacisnął przycisk krótko (< 2.5 s)
  - `USER_REJECT_BIT` (bit 1): użytkownik przytrzymał przycisk długo (≥ 2.5 s)
- **Producent:** `task_user.c` (logika debouncingu + timing)
- **Konsument:** `task_sign.c` (czeka na bity zdarzeń przez `xEventGroupWaitBits()`)
- **Cel:** nieblokujący sygnał z obsługi przycisku

### Semafory

```c
g_i2c_mutex = xSemaphoreCreateMutex();    // magistrala I2C1
g_ui_mutex = xSemaphoreCreateMutex();      // bufor tekstu UI
g_display_ctx_mutex = xSemaphoreCreateMutex(); // display_context_t
```

| Semafor | Chroni | Zadania | Timeout |
|---|---|---|---|
| `g_i2c_mutex` | Sprzęt I2C1 (GPIO AF, DR, status) | task_display, task_io | Nieskończony (brak ryzyka inwersji priorytetu) |
| `g_ui_mutex` | Sformatowany bufor tekstu do wyświetlacza | task_display, task_net, task_sign | Nieskończony |
| `g_display_ctx_mutex` | Struktura `g_display_ctx` (4 linie tekstu, kursor) | task_display | Nieskończony |

**Uwaga projektowa:** nieskończony timeout jest tutaj dopuszczalny, ponieważ:
- Każde zadanie trzyma semafor krótko (< 1 ms)
- Żadne zadanie nie wykonuje operacji blokujących I2C trzymając semafor
- Brak zagnieżdżonego uchwycenia semafora (brak ryzyka deadlocku)

### Globalne flagi i bufory

```c
volatile uint8_t  g_security_alert = 0;     // Ustawione przez task_sign przy błędzie walidacji
uint8_t          g_last_sig[64] = {0};      // Ostatni podpis ECDSA (r || s)
volatile uint8_t  g_last_sig_ready = 0;    // Flaga: podpis gotowy do odpowiedzi USB/HTTP
```

- **`g_security_alert`:** atomowa flaga czytana przez task_io do zapalenia LED3
- **`g_last_sig`:** bufor wspólnego podpisu; chroniony przez niejawne porządkowanie FSM (zadanie podpisywania pisze, zadanie sieciowe czyta po `g_last_sig_ready` = 1)
- **`g_last_sig_ready`:** volatile flaga do sygnalizowania gotowości podpisu bez konieczności semafora

## Zależności i relacje

### Warstwa sprzętu (wczesna inicjalizacja)

```
main.c
 ├─ HW_Init_Early_LwIP()   → MPU, config pamięci podręcznej
 ├─ HW_Init()              → GPIO, I2C, UART, USB, RNG
 └─ hw_init.c, hw_init.h   → konfiguracja specyficzna dla urządzenia
```

### Czas i entropia

```
main.c
 ├─ time_service_init()     → synchronizacja SNTP, formatowanie UTC
 ├─ crypto_rng_init()       → wysiewanie puli entropii trezor-crypto [jeśli USE_CRYPTO_SIGN]
 └─ time_service.c, crypto_wallet.c
```

### Tworzenie zadań i przesyłanie wiadomości

```
main.c
 ├─ Task_Display_Create()   ──┐
 ├─ Task_Net_Create()       ──┼─ Zależą od:
 ├─ Task_Sign_Create()      ──┤  • g_tx_queue
 ├─ Task_IO_Create()        ──┤  • g_display_queue
 ├─ Task_User_Create()      ──┤  • g_user_event_group
 └─ (Task_Security nie utworzana)  • g_*_mutex
```

### Globalne kontrakty (wallet_shared.h)

```c
// Zadeklarowane w main.c, zadeklarowane extern w wallet_shared.h
QueueHandle_t      g_tx_queue;
QueueHandle_t      g_display_queue;
EventGroupHandle_t g_user_event_group;
SemaphoreHandle_t  g_i2c_mutex;
SemaphoreHandle_t  g_ui_mutex;
SemaphoreHandle_t  g_display_ctx_mutex;
display_context_t  g_display_ctx;
volatile uint8_t   g_security_alert;
uint8_t            g_last_sig[64];
volatile uint8_t   g_last_sig_ready;
```

Wszystkie zadania zawierają `wallet_shared.h` i uzyskują dostęp za pośrednictwem deklaracji extern.

## Moduły powiązane

### Bezpośredni konsumenci (utworzeni przez main)

- **`task_display.md`** — zadanie wyświetlacza otrzymuje zdarzenia `g_display_queue`
- **`task_net.md`** — zadanie sieciowe umieszcza w kolejkach `g_tx_queue` i `g_display_queue`
- **`task_sign.md`** — FSM podpisywania konsumuje `g_tx_queue`, ustawia flagi podpisu
- **`task_io.md`** — zadanie wskaźnika czyta `g_security_alert`, zarządzanie stanem LED
- **`task_user.md`** — obsługa przycisku sygnalizuje poprzez `g_user_event_group`

### Infrastruktura

- **`hw_init.md`** — kolejność inicjalizacji sprzętu i uzasadnienie
- **`time_service.md`** — zarządzanie czasem SNTP i UTC
- **`wallet_shared.md`** — definicje typów IPC i globalne kontrakty
- **`crypto_wallet.md`** — inicjalizacja entropii (jeśli USE_CRYPTO_SIGN)

### Powiązane (nie utworzone tutaj)

- **`task_security.c`** — alternatywny FSM podpisywania; połączony, ale nie instancjonowany
- **`app_ethernet_cw.md`** — zarządzanie łączem Ethernet (wywoływane z task_net)
- **`usb_device.md`**, **`usb_webusb.md`** — interfejs urządzenia USB (zainicjalizowany w hw_init)

## Krytyczne uwagi projektowe

### 1. Dlaczego wczesna inicjalizacja LwIP?

Bufory DMA LwIP muszą być w regionie pamięci ze specyficznym politykami pamięci podręcznej:
- Write-through dla buforów TX (unikanie problemów spójności)
- Cacheable dla buforów RX (wydajność)

MPU (Memory Protection Unit) musi być skonfigurowany **przed** `HAL_Init()` tak, aby gdy `HAL_Init()` włączy D-Cache, zachowanie pamięci podręcznej pasowało do regionów MPU. To jest replikowane z przykładów `lwip_zero` i `lwip-uaid-SSD1306` w STM32CubeH7.

### 2. Globalne kolejki vs. callback

Dlaczego nie używać powiadomień zadań lub callback?

- **Kolejki buforują:** jeśli zadanie sieciowe umieści 2 transakcje w kolejce przed uruchomieniem zadania podpisywania, obie są w kolejce (brak straty).
- **Rozdzielenie:** zadanie sieciowe nie zna i nie obchodzi czasowanie zadania podpisywania; po prostu umieszcza w kolejce i kontynuuje.
- **Deterministyczne porządkowanie:** semantyka FIFO jest jasna i przewidywalna.

Dla jednorazowych sygnałów (naciśnięcie przycisku) grupy zdarzeń są bardziej efektywne niż kolejki.

### 3. Dlaczego Task_Security nie jest tworzona

`task_security.c` implementuje alternatywny FSM podpisywania z pozorną kryptografią. Jest połączony, ale nie instancjonowany (`Task_Security_Create()` nigdy nie jest wywoływane), ponieważ:
- **Budowanie produkcji** używa `task_sign.c` z rzeczywistym ECDSA
- **Tylko audyt/test** mogą instancjonować `task_security.c` do porównania zachowania podpisywania
- Utrzymywanie obu połączonych pozwala elastyczność czasu kompilacji bez zaśmiecania main.c

### 4. Strategia obsługi błędów

Trzy hakie fatalne łapią nieodwracalne błędy:
- `vApplicationMallocFailedHook()` — sterta wyczerpana
- `vApplicationStackOverflowHook()` — przepełnienie stosu zadania
- `Error_Handler()` — catch-all (również wywoływane jeśli tworzenie kolejki się nie powiedzie)

Wszystkie trzy logują do UART i pętlą w nieskończoność. To jest zamierzone: w portfelu osadzonym detekcja awarii (logowanie) i bezpieczne zatrzymanie jest ważniejsze niż eleganckie odzyskanie.

### 5. Logowanie wyświetlacza podczas rozruchu

`Task_Display_Log()` jest wywoływane w całym `main.c` **przed** stworzeniem zadania wyświetlacza. To działa, ponieważ:
- `Task_Display_Log()` zapisuje bezpośrednio na SSD1306 i UART
- Nie zależy od uruchomionego zadania; to po prostu wызów funkcji
- Wczesny wyjście diagnostyczne jest bezcenne do debugowania zawieszenia rozruchu
