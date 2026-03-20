# CryptoWallet

Aplikacja mikrokontrolera do bezpiecznego podpisywania transakcji Bitcoin na STM32H743. Integruje trezor-crypto, FreeRTOS, LwIP, SSD1306 UI i WebUSB.

**Dokumentacja dostępna w:**
- 🇬🇧 [English](README.md)
- 🇷🇺 [Русский](README_ru.md)
- 🇵🇱 [Polski](README_pl.md) (ten plik)

---

## 🔄 Architektura Kernel & System Software

### Organizacja Kernel FreeRTOS

**Takt systemowy i planowanie:**
- **Wyjątek SysTick**: Skonfigurowany do uruchamiania co 1 ms
- **Przepływ obsługi taktu:**
  1. Zapisz stan CPU (rejestry zapisane przez sprzęt Cortex-M7)
  2. Wywołaj `xTaskIncrementTick()` → zaktualizuj globalny licznik taktów
  3. Oceń gotowość wszystkich zadań z wygasłymi opóźnieniami
  4. Znajdź gotowe zadanie z najwyższym priorytetem
  5. Jeśli inne niż bieżące → zaznacz wyjątek PendSV
  6. Powróć z przerwania
  7. Wyjątek PendSV → przełączenie kontekstu (zapisz stary, załaduj nowy)
- **Czas przełączenia kontekstu**: ~5-10 mikrosekund (przyspieszony sprzętowo)
- **Kontekst zawiera**: 16 rejestrów (R0-R15), XPSR (flagi), rejestr sterowania

**Kolejka gotowości zadań:**
- **Struktura danych**: 32 połączone listy (jeden na poziom priorytetu 0-31)
- **O(1) wyszukiwanie**: Gotowe zadanie o najwyższym priorytecie jest natychmiast znane
- **Lock-free** (w kontekście ISR): Używa atomowych operacji na polach bitowych
- **Przykład**: Jeśli priorytety 22, 21, 20 mają gotowe zadania, harmonogram wybiera 22

**Mechanizm opóźnienia zadań:**
- **Lista timerów**: Posortowana po czasie przebudzenia
- **Obsługa przepełnienia**: Używa 16-bitowego owijania timera (FreeRTOS v10+)
- **Przepływ przebudzenia**: Lista timerów → kolejka gotowości → następna ocena SysTick

**Dziedziczenie priorytetu (Mutex):**
- **Problem**: Inwersja priorytetu (niski priorytet trzymający mutex blokuje wysoki)
- **Rozwiązanie**: Gdy zadanie czeka na mutex, tymczasowo podnieś priorytet właściciela blokady
- **Przykład w kodzie**: `crypto_lock` mutex używa dziedziczenia priorytetu
  - Jeśli task_net czeka na crypto_lock, task_sign otrzymuje podwyższony priorytet
  - Po zwolnieniu przez task_sign, priorytet spada z powrotem

### Wzorce synchronizacji zadań

**Komunikacja oparta na kolejce (tx_request_queue → task_sign):**
```c
// Producent (task_net):
xQueueSend(tx_request_queue, &tx_request, portMAX_DELAY);
// Blokuje jeśli kolejka pełna (zwykle nie, głębokość=10)

// Konsument (task_sign):
xQueueReceive(tx_request_queue, &tx_request, portMAX_DELAY);
// Blokuje do otrzymania wiadomości
// Atomowo usuwa wiadomość z kolejki
// Budzi się nawet jeśli inne zadania wyższe priorytety (bo mają dane)
```

**Grupa zdarzeń (user_confirm_event):**
```c
// Czekający (task_sign):
xEventGroupWaitBits(user_confirm_event, CONFIRM_BIT, 
                    pdTRUE,      // wyczyść przy wyjściu
                    pdTRUE,      // czekaj wszystkie bity
                    xTicksToWait); // timeout: 30 sekund

// Sygnalizujący (task_user):
xEventGroupSetBitsFromISR(user_confirm_event, CONFIRM_BIT, &xHigherPriorityTaskWoken);
```

**Mutex z timeoutem (crypto_lock):**
```c
// Zadanie zabiera blokadę z timeoutem 5 sekund:
if (xSemaphoreTake(crypto_lock, pdMS_TO_TICKS(5000)) == pdTRUE) {
    // Wykonaj operację kryptograficzną
    xSemaphoreGive(crypto_lock);
} else {
    // Timeout - inne zadanie trzyma blokadę zbyt długo
    // Akcja: błąd odpowiedzi lub ponowna próba
}
```

### Analiza ograniczeń czasu rzeczywistego

**Hard Real-Time (nie można pominąć):**
- **SysTick** → termin 1 ms (sprzęt)
- **Opóźnienie przerwania** → maksymalnie ~10 µs (ograniczenie rdzenia ARM)
- **Sekcja krytyczna**: Wyłączone przerwania ≤ 100 µs (zapobiega długiemu jitterowi)

**Soft Real-Time (opuszczone terminy degradują, nie krytyczne):**
- **Odpowiedź na naciśnięcie przycisku**: ≤ 100 ms dopuszczalne (debounce + interwał)
- **Wyświetlacz OLED**: ≤ 100 ms dopuszczalne (postrzeganie oka)
- **Odpowiedź HTTP**: ≤ 1 sekunda dopuszczalna (interakcja użytkownika)

**Analiza blokowania zadań:**
- **task_sign zablokowana w WAIT_USER**: Zadanie najwyższego priorytetu zablokowane
  - Inne zadania działają: task_net, task_display, task_io, task_user
  - Ruch sieciowy nadal obsługiwany (przez task_net)
  - Brak zagłodzenia (wszystkie zadania otrzymują czas CPU)
- **Planowanie po priorytecie gwarantuje**:
  - Naciśnięcie przycisku (task_user, priorytet 22) zawsze wyparcia inne
  - Podpisywanie (priorytet 21) wyparcia sieć (priorytet 20)
  - Sieć nie głoduje niższych priorytetów

---

## 🔐 Jądro i bezpieczeństwo

Logika podpisywania, zarządzanie kluczami, walidacja i operacje kryptograficzne.

### [main.c](Core/Src/main.c) — Entry point and application management
FreeRTOS entry point: initialization of IPC objects (queues, semaphores, event groups), creation and startup of critical tasks.
**Pełna dokumentacja:** [docs_src/main.md](docs_src/main.md)

### [task_sign.c](Core/Src/task_sign.c) — Signing pipeline (FSM)
The workhorse: validates request from queue, forms SHA-256, awaits user confirmation, ECDSA signature, saves result.
**Pełna dokumentacja:** [docs_src/task_sign.md](docs_src/task_sign.md)

### [crypto_wallet.c](Core/Src/crypto_wallet.c) — Cryptographic layer (trezor-crypto)
Wrapper over trezor-crypto: STM32 RNG + entropy pooling, BIP-39 mnemonics, BIP-32 HD derivation (m/44'/0'/0'/0/0), ECDSA secp256k1.
**Pełna dokumentacja:** [docs_src/crypto_wallet.md](docs_src/crypto_wallet.md)

### [tx_request_validate.c](Core/Src/tx_request_validate.c) — Validation gate
Guardian layer before signing: validates address (Base58/bech32), amount (decimal), currency (whitelist).
**Pełna dokumentacja:** [docs_src/tx_request_validate.md](docs_src/tx_request_validate.md)

### [memzero.c](Core/Src/memzero.c) — Secure zeroing
Destructs sensitive buffers (keys, digests, seeds) via volatile writes, preventing compiler optimization.
**Pełna dokumentacja:** [docs_src/memzero.md](docs_src/memzero.md)

### [sha256_minimal.c](Core/Src/sha256_minimal.c) — SHA-256 fallback
Compact SHA-256 implementation (when `USE_CRYPTO_SIGN=0` without trezor-crypto).
**Pełna dokumentacja:** [docs_src/sha256_minimal.md](docs_src/sha256_minimal.md)

### [wallet_seed.c](Core/Src/wallet_seed.c) — Seed management (test)
Test seed for development (`USE_TEST_SEED=1`): BIP-39 vector "abandon...about", **development only**.
**Pełna dokumentacja:** [docs_src/wallet_seed.md](docs_src/wallet_seed.md)

### [task_security.c](Core/Src/task_security.c) — Legacy signing (audit/test)
Alternative signing FSM with mock cryptography for bring-up and comparison.
**Pełna dokumentacja:** [docs_src/task_security.md](docs_src/task_security.md)

**Nagłówki:** crypto_wallet.h, memzero.h, sha256_minimal.h, task_sign.h, task_security.h, tx_request_validate.h, wallet_shared.h

---

## 📡 Interfejsy komunikacyjne

Stos sieciowy (LwIP/Ethernet), USB (WebUSB), synchronizacja czasu.

### [task_net.c](Src/task_net.c) — HTTP server and network API
LwIP/Ethernet startup, HTTP on port 80, JSON/form `POST /tx` parsing, validation, enqueue to `g_tx_queue`.
**Pełna dokumentacja:** [docs_src/task_net.md](docs_src/task_net.md)

### [usb_webusb.c](Core/Src/usb_webusb.c) — WebUSB vendor interface
Vendor-specific WebUSB: bulk endpoints, ping/pong, binary frame for signature request.
**Pełna dokumentacja:** [docs_src/usb_webusb.md](docs_src/usb_webusb.md)

### [app_ethernet_cw.c](Src/app_ethernet_cw.c) — Ethernet link and DHCP FSM
Ethernet link callback for up/down, DHCP state machine (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback.
**Pełna dokumentacja:** [docs_src/app_ethernet_cw.md](docs_src/app_ethernet_cw.md)

### [time_service.c](Core/Src/time_service.c) — SNTP and UTC
Time synchronization via SNTP, unified access to Unix epoch and UTC strings.
**Pełna dokumentacja:** [docs_src/time_service.md](docs_src/time_service.md)

### [usb_device.c](Core/Src/usb_device.c) — USB device initialization
HSI48 clock configuration, USBD core initialization, WebUSB class registration.
**Pełna dokumentacja:** [docs_src/usb_device.md](docs_src/usb_device.md)

### [usbd_conf_cw.c](Core/Src/usbd_conf_cw.c) — USB BSP configuration
Static allocator for USBD, MSP for PCD (GPIO AF, clock, NVIC), HAL_PCD bridging to USBD_LL.
**Pełna dokumentacja:** [docs_src/usbd_conf_cw.md](docs_src/usbd_conf_cw.md)

### [usbd_desc_cw.c](Core/Src/usbd_desc_cw.c) — USB descriptors
Device/interface/BOS descriptors, strings (manufacturer, product, serial), WebUSB Platform Capability UUID.
**Pełna dokumentacja:** [docs_src/usbd_desc_cw.md](docs_src/usbd_desc_cw.md)

**Nagłówki:** task_net.h, usb_device.h, usb_webusb.h, usbd_conf.h, usbd_conf_cw.h, usbd_desc_cw.h, app_ethernet.h, time_service.h, lwipopts.h

---

## 🎨 Doświadczenie użytkownika

Zarządzanie wyświetlaczem, obsługa przycisków, zdarzenia systemowe i wskaźniki.

### [task_display.c](Core/Src/task_display.c) — SSD1306 UI (full version)
Visual state management on SSD1306 128×32: 4 lines, scrolling log, state merging.
**Pełna dokumentacja:** [docs_src/task_display.md](docs_src/task_display.md)

### [task_display_minimal.c](Core/Src/task_display_minimal.c) — SSD1306 UI (minimal)
Minimal UI for `minimal-lwip`: UART mirroring, periodic display updates.
**Pełna dokumentacja:** [docs_src/task_display_minimal.md](docs_src/task_display_minimal.md)

### [task_user.c](Core/Src/task_user.c) — User button (PC13)
Physical UX: debouncing, short press (Confirm) vs long hold ~2.5s (Reject).
**Pełna dokumentacja:** [docs_src/task_user.md](docs_src/task_user.md)

### [task_io.c](Core/Src/task_io.c) — LED indicators
Visual indicators: LED1 = alive heartbeat, LED2 = network activity, LED3 = security alert.
**Pełna dokumentacja:** [docs_src/task_io.md](docs_src/task_io.md)

**Nagłówki:** task_display.h, task_user.h, task_io.h

---

## ⚙️ System i sprzęt

Inicjalizacja HAL, taktowanie, procedury obsługi przerwań, konfiguracja sterownika.

### [hw_init.c](Core/Src/hw_init.c) — Board bring-up (clocks, MPU, GPIO, I2C, UART)
Low-level bootstrap: clock configuration, MPU/cache (for LwIP), GPIO (LED, button), I2C1 (OLED), UART, USB, RNG.
**Pełna dokumentacja:** [docs_src/hw_init.md](docs_src/hw_init.md)

### [stm32h7xx_hal_msp.c](Core/Src/stm32h7xx_hal_msp.c) — HAL MSP callbacks
Hardware-level configuration: `HAL_I2C_MspInit` (I2C1), `HAL_UART_MspInit` (USART3).
**Pełna dokumentacja:** [docs_src/stm32h7xx_hal_msp.md](docs_src/stm32h7xx_hal_msp.md)

### [stm32h7xx_it.c](Core/Src/stm32h7xx_it.c) — Interrupt handlers (main)
Handlers: `SysTick_Handler` (FreeRTOS tick), Ethernet IRQ (when `USE_LWIP`).
**Pełna dokumentacja:** [docs_src/stm32h7xx_it.md](docs_src/stm32h7xx_it.md)

### [stm32h7xx_it_systick.c](Core/Src/stm32h7xx_it_systick.c) — SysTick for minimal-lwip
Alternative `SysTick_Handler` for `minimal-lwip` build.
**Pełna dokumentacja:** [docs_src/stm32h7xx_it_systick.md](docs_src/stm32h7xx_it_systick.md)

### [stm32h7xx_it_usb.c](Core/Src/stm32h7xx_it_usb.c) — USB OTG HS IRQ
OTG HS interrupt handler: calls `HAL_PCD_IRQHandler` for WebUSB.
**Pełna dokumentacja:** [docs_src/stm32h7xx_it_usb.md](docs_src/stm32h7xx_it_usb.md)

### [ssd1306_conf.h](Drivers/ssd1306/ssd1306_conf.h) — Display driver configuration
Build-time parameters: I2C1 binding, address 0x3C, geometry 128×32, font 6×8.
**Pełna dokumentacja:** [docs_src/ssd1306_conf.md](docs_src/ssd1306_conf.md)

**Nagłówki:** hw_init.h, main.h, lwipopts.h

---

## 💾 Stos System Software

**Architektura warstwowa:**
```
┌─────────────────────────────────────┐
│  Warstwa aplikacji (task_*.c)       │ ← Kod zadań
│  - Podpisywanie, Sieć, Wyświetlacz  │
├─────────────────────────────────────┤
│  Warstwa Middleware                 │
│  - LwIP (sieć)                      │
│  - trezor-crypto (kryptografia)     │
│  - FreeRTOS (jądro)                 │
├─────────────────────────────────────┤
│  Warstwa HAL (hal/)                 │ ← Sterowniki
│  - UART, GPIO, I2C, SPI, USB, Eth   │
├─────────────────────────────────────┤
│  Warstwa CMSIS (stm32h7xx_hal_*.h)  │ ← Definicje rejestrów
├─────────────────────────────────────┤
│  Rdzeń ARM Cortex-M7                │ ← CPU
│  - Pamięć cache, MPU, FPU, itd      │
└─────────────────────────────────────┘
```

**Zależności modułów:**
```
task_sign.c
├─ FreeRTOS (queue, mutex, event)
├─ crypto_wallet.c
│  └─ trezor-crypto
│     └─ secp256k1, BIP-39/32
└─ memzero.c (bezpieczne czyszczenie bufora)

task_net.c
├─ FreeRTOS (queue)
├─ LwIP
│   ├─ Stos TCP/IP
│   ├─ Klient DHCP
│   └─ Serwer HTTP
└─ eth_phy_driver.c

task_display.c
├─ FreeRTOS (queue)
└─ i2c_driver.c → SSD1306 OLED

task_user.c
├─ FreeRTOS (event, queue)
└─ gpio_driver.c → GPIO_PC13 (przycisk)
```

---

## 🔒 Uwagi dotyczące bezpieczeństwa (poziom systemowy)

**Bezpieczeństwo pamięci:**
- **Ochrona przed przepełnieniem stosu**: Stałe rozmiary stosu zapobiegają niekontrolowanemu wzrostowi
- **Fragmentacja sterty**: Oddzielna sterta zmniejsza przewidywalność stanu stosu
- **Dane wrażliwe**: Czyszczone za pośrednictwem volatile pisania (zapobiega optymalizacji kompilatora)

**Bezpieczeństwo czasu rzeczywistego:**
- **Ataki czasowe**: Operacje kryptograficzne są zmienne z RNG (timing nieprzewidywalny)
- **Analiza energii**: Trudna (wymagany oscyloskop, niepraktyczne dla przeglądu kodu)
- **Iniekcja błędów**: Wymaga fizycznego dostępu (sprzęt do glitchingu)

**Bezpieczeństwo wykonania:**
- **Bez randomizacji adresów** (ograniczenie embedded)
- **Bez bitu DEP/NX** (ograniczenie Cortex-M7)
- **Weryfikacja bootloadera** (jeśli włączono): Gwarantuje tylko autoryzowany kod

**Bezpieczeństwo IPC:**
- **Przepełnienie kolejki**: Statycznie przydzielane (bez błędu przydziału)
- **Deadlock mutex**: Dziedziczenie priorytetu FreeRTOS zapobiega
- **Izolacja zadań**: Brak (wszystkie zadania dzielą przestrzeń adresową, przez projekt)

---

## 📚 Dokumentacja i materiały referencyjne

- **[docs_src/README.md](docs_src/README.md)** — Complete index of all 32 modules with hierarchical navigation
- **[docs_src/doxygen-comments.md](docs_src/doxygen-comments.md)** — Doxygen comment style guidelines
- **[docs_src/api-documentation-scope.md](docs_src/api-documentation-scope.md)** — Documentation progress tracking

---

## 🚀 Szybki start

### Struktura dokumentacji
1. **Kod (.c/.h)** — minimalne @brief/@details, poziom API
2. **docs_src/*.md** — szczegółowe wyjaśnienia logiki (Abstract → Logic Flow → Dependencies)
3. **Doxygen HTML** — odsyłacze krzyżowe kodu (`make docs-doxygen`)

### Jak czytać moduł
1. Open [docs_src/README.md](docs_src/README.md)
2. Znajdź interesujący Cię moduł
3. Zacznij od **Abstract** (logika biznesowa) → **Logic Flow** (algorytm) → **Dependencies**
4. Śledź **Relations** dla szerszego kontekstu

### Główne polecenia
```bash
make docs-doxygen    # Generate Doxygen
make build          # Build
make minimal-lwip   # Minimal build
make flash          # Flash to STM32
```

---

## 📋 Pełny indeks modułów

<!-- DOXYGEN_DOCS_SRC_INDEX -->
| Модуль | Краткий обзор |
|--------|------------------|
| [app_ethernet](docs_src/app_ethernet.md) | <brief>The `app_ethernet` header defines Ethernet "glue" layer interfaces: it declares callback `ethernet_link_status_updated()`, LED state reading functions, and when `LWIP_DHCP` enabled, declares `DHCP_Thread()` and set of DHCP FSM state constants used by implementation in `Src/app_ethernet_cw.c`.</brief> |
| [app_ethernet_cw](docs_src/app_ethernet_cw.md) | <brief>The `app_ethernet_cw` module provides Ethernet support: FSM for link-up/link-down states, DHCP client (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), IP address logging.</brief> |
| [app_ethernet_cw_pl](docs_src/app_ethernet_cw_pl.md) | <brief>Moduł `app_ethernet_cw` zapewnia obsługę Ethernet: FSM dla stanów link-up/link-down, klient DHCP (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), informacja zwrotna LED (LED2 = sieć, LED3 = link-down), rejestrowanie adresu IP.</brief> |
| [app_ethernet_cw_ru](docs_src/app_ethernet_cw_ru.md) | Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса. |
| [app_ethernet_pl](docs_src/app_ethernet_pl.md) | <brief>Nagłówek `app_ethernet` definiuje interfejsy warstwy Ethernet "glue": deklaruje callback `ethernet_link_status_updated()`, funkcje odczytu bieżących stanów LED i, gdy `LWIP_DHCP` włączony, deklaruje `DHCP_Thread()` i zestaw stałych stanu FSM DHCP używanych przez implementację w `Src/app_ethernet_cw.c`.</brief> |
| [app_ethernet_ru](docs_src/app_ethernet_ru.md) | Заголовок `app_ethernet` задаёт интерфейсы Ethernet "glue" слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`. |
| [architecture](docs_src/architecture.md) | ## Project Summary |
| [architecture_pl](docs_src/architecture_pl.md) | ## Streszczenie projektu |
| [architecture_ru](docs_src/architecture_ru.md) | ## Краткое описание проекта |
| [AUTOGENERATION_SYSTEM](docs_src/AUTOGENERATION_SYSTEM.md) | Это краткое описание модуля... ``` |
| [doxygen-comments](docs_src/doxygen-comments.md) | У Doxygen нет нативной опции `GENERATE_MARKDOWN`, поэтому этот проект использует **XML output** и скрипты для генерации Markdown и обновления README. Чтобы корректно разделять **короткое** (`@brief`) и **длинное** (`@details`) описание, в коде применяется единый шаблон комментариев. |
| [hw_init](docs_src/hw_init.md) | <brief>The `hw_init` module handles low-level board bring-up: it establishes proper clock/cache/MPU ordering (critical for LwIP/ETH), initializes GPIO for UX (LED/button), sets up I2C1 for SSD1306 and UART logging, plus optional USB and RNG.</brief> |
| [hw_init_pl](docs_src/hw_init_pl.md) | <brief>Moduł `hw_init` obsługuje inicjalizację sprzętu niskiego poziomu: ustanawia prawidłową kolejność zegara/pamięci podręcznej/MPU (krytyczne dla LwIP/ETH), inicjalizuje GPIO dla UX (LED/przycisk), konfiguruje I2C1 dla SSD1306 i logowania UART, plus opcjonalnie USB i RNG.</brief> |
| [hw_init_ru](docs_src/hw_init_ru.md) | <brief>Модуль `hw_init` отвечает за низкоуровневый bring-up платы: он формирует правильный порядок тактов/кэша/MPU (критично для LwIP/ETH), инициализирует GPIO для UX (LED/кнопка), поднимает I2C1 под SSD1306 и UART-лог, а также опционально поднимает USB и RNG.</brief> |
| [lwipopts](docs_src/lwipopts.md) | <brief>Header `lwipopts` defines compile-time configuration for LwIP: enables IPv4/TCP/DHCP/DNS/SNTP, configures heap size (`LWIP_RAM_HEAP_POINTER` and `MEM_SIZE`), TCP buffer/window parameters, and binds SNTP time updates to `time_service_set_epoch()`.</brief> |
| [lwipopts_pl](docs_src/lwipopts_pl.md) | <brief>Nagłówek `lwipopts` definiuje opcje konfiguracji czasu kompilacji dla LwIP: włącza IPv4/TCP/DHCP/DNS/SNTP, konfiguruje rozmiar stosu (`LWIP_RAM_HEAP_POINTER` i `MEM_SIZE`), parametry bufora TCP/okna, wiąże aktualizacje SNTP z `time_service_set_epoch()`.</brief> |
| [lwipopts_ru](docs_src/lwipopts_ru.md) | <brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер кучи (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, привязывает обновления SNTP к `time_service_set_epoch()`.</brief> |
| [main](docs_src/main.md) | <brief>The `main` module is the system orchestrator: it bootstraps hardware (HAL, LwIP early init, clocks), establishes IPC contracts (queues, semaphores, event groups, global state), initializes time/crypto services, and spawns the core FreeRTOS task family (display, network, signing, IO, user input). Entry point for the entire embedded wallet application.</brief> |
| [main_pl](docs_src/main_pl.md) | <brief>Moduł `main` jest orkiestratorem systemu: inicjalizuje sprzęt (HAL, wczesna inicjalizacja LwIP, taktowanie), ustanawia kontrakty IPC (kolejki, semafory, grupy zdarzeń, stan globalny), inicjalizuje usługi czasu/kryptografii oraz uruchamia podstawową rodzinę zadań FreeRTOS (wyświetlacz, sieć, podpis, IO, wejście użytkownika). Punkt wejścia całej aplikacji portfela osadzonego.</brief> |
| [main_ru](docs_src/main_ru.md) | <brief>Модуль `main` — это системный оркестратор приложения: инициализирует железо (HAL, ранняя инициализация LwIP, тактирование), устанавливает контракты IPC (очереди, семафоры, группы событий, глобальное состояние), инициализирует сервисы времени/криптографии, и запускает семейство основных задач FreeRTOS (дисплей, сеть, подпись, IO, ввод пользователя). Точка входа для всего встроенного приложения кошелька.</brief> |
| [README](docs_src/README.md) | Здесь лежат **развёрнутые текстовые разборы** логики кода — дополнение к комментариям в исходниках и к выводу Doxygen (`make docs-doxygen`). |
| [reference-code](docs_src/reference-code.md) | !!! warning "Generated file" Do not edit by hand. Regenerate: |
| [ssd1306_conf](docs_src/ssd1306_conf.md) | <brief>Header `ssd1306_conf` defines build-time configuration for the SSD1306 driver: binds I2C1 (`hi2c1`) and address 0x3C, sets display geometry 128×32, and enables the required font (Font 6×8). These macros are consumed by driver sources and calling UI code.</brief> |
| [ssd1306_conf_pl](docs_src/ssd1306_conf_pl.md) | <brief>Nagłówek `ssd1306_conf` definiuje konfigurację czasu kompilacji dla sterownika SSD1306: wiąże I2C1 (`hi2c1`) i adres 0x3C, ustawia geometrię wyświetlacza 128×32, włącza wymagany czcionkę (Font 6×8). Te makra są używane przez źródła sterownika i kod UI wywołujący.</brief> |
| [ssd1306_conf_ru](docs_src/ssd1306_conf_ru.md) | <brief>Заголовок `ssd1306_conf` задаёт build-time конфигурацию для драйвера SSD1306: привязывает I2C1 (`hi2c1`) и адрес 0x3C, устанавливает геометрию дисплея 128×32, включает требуемый шрифт (Font 6×8). Эти макросы потребляются исходниками драйвера и вызывающим UI-кодом.</brief> |
| [stm32h7xx_hal_msp](docs_src/stm32h7xx_hal_msp.md) | <brief>File `stm32h7xx_hal_msp` sets up hardware layer for HAL: `HAL_I2C_MspInit` configures I2C1 (PB8/PB9 as AF OD with pull-up), and `HAL_UART_MspInit` configures USART3 (TX/RX pins and clocks). This ensures `hw_init` can initialize I2C peripherals and UART without manual pin configuration in the application.</brief> |
| [stm32h7xx_hal_msp_pl](docs_src/stm32h7xx_hal_msp_pl.md) | <brief>Plik `stm32h7xx_hal_msp` konfiguruje warstwę sprzętu dla HAL: `HAL_I2C_MspInit` konfiguruje I2C1 (PB8/PB9 jako AF OD z pull-up), a `HAL_UART_MspInit` konfiguruje USART3 (piny TX/RX i zegary). Zapewnia to, że `hw_init` może inicjalizować peryferia I2C i UART bez ręcznej konfiguracji pinów w aplikacji.</brief> |
| [stm32h7xx_hal_msp_ru](docs_src/stm32h7xx_hal_msp_ru.md) | <brief>Файл `stm32h7xx_hal_msp` задаёт "железную" часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 как AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` может инициализировать I2C периферию и UART без ручной конфигурации пинов в приложении.</brief> |
| [stm32h7xx_it](docs_src/stm32h7xx_it.md) | <brief>File `stm32h7xx_it` implements critical interrupt handlers for the main scenario: `SysTick_Handler` bridges HAL tick to FreeRTOS tick, and when LwIP is enabled, processes Ethernet IRQ via `HAL_ETH_IRQHandler`.</brief> |
| [stm32h7xx_it_pl](docs_src/stm32h7xx_it_pl.md) | <brief>Plik `stm32h7xx_it` implementuje krytyczne procedury obsługi przerwań dla głównego scenariusza: `SysTick_Handler` łączy zegar HAL z zegarem FreeRTOS, a gdy LwIP jest włączony, obsługuje IRQ Ethernet za pośrednictwem `HAL_ETH_IRQHandler`.</brief> |
| [stm32h7xx_it_ru](docs_src/stm32h7xx_it_ru.md) | <brief>Файл `stm32h7xx_it` реализует критичные обработчики прерываний для основного сценария: `SysTick_Handler` связывает HAL tick с FreeRTOS tick, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief> |
| [stm32h7xx_it_systick](docs_src/stm32h7xx_it_systick.md) | <brief>File `stm32h7xx_it_systick` addresses the situation for minimal-lwip builds: it provides `SysTick_Handler`, which synchronizes HAL tick and FreeRTOS tick to avoid hangs/incorrect behavior when the base `stm32h7xx_it.c` does not contain `SysTick_Handler`.</brief> |
| [stm32h7xx_it_systick_pl](docs_src/stm32h7xx_it_systick_pl.md) | <brief>Plik `stm32h7xx_it_systick` rozwiązuje sytuację dla kompilacji minimal-lwip: udostępnia `SysTick_Handler`, który synchronizuje zegar HAL i zegar FreeRTOS, aby uniknąć zawieszenia/nieprawidłowego zachowania, gdy podstawowy `stm32h7xx_it.c` nie zawiera `SysTick_Handler`.</brief> |
| [stm32h7xx_it_systick_ru](docs_src/stm32h7xx_it_systick_ru.md) | <brief>Файл `stm32h7xx_it_systick` разрешает ситуацию для minimal-lwip сборок: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и FreeRTOS tick, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief> |
| [stm32h7xx_it_usb](docs_src/stm32h7xx_it_usb.md) | <brief>File `stm32h7xx_it_usb` implements the OTG HS interrupt handler for WebUSB mode: the ISR calls `HAL_PCD_IRQHandler` for `hpcd_USB_FS` so USB device middleware can properly service endpoint transfers.</brief> |
| [stm32h7xx_it_usb_pl](docs_src/stm32h7xx_it_usb_pl.md) | <brief>Plik `stm32h7xx_it_usb` implementuje procedurę obsługi przerwania OTG HS dla trybu WebUSB: ISR wywołuje `HAL_PCD_IRQHandler` dla `hpcd_USB_FS`, aby oprogramowanie pośrednie USB device mogło prawidłowo obsługiwać transfery endpoint'ów.</brief> |
| [stm32h7xx_it_usb_ru](docs_src/stm32h7xx_it_usb_ru.md) | <brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: ISR вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог правильно обслуживать transfers endpoint'ов.</brief> |
| [task_display](docs_src/task_display.md) | <brief>The `task_display` module manages the visual state of the wallet on SSD1306: it receives network/signing events, merges them into a single displayable state, and renders 4 lines of UI with "scrolling text" for logs and network data.</brief> |
| [task_display_minimal](docs_src/task_display_minimal.md) | <brief>The `task_display_minimal` module is a lightweight UI/logging implementation for `minimal-lwip`: it minimizes SSD1306 load, mirrors messages to UART, and writes a short log tail to `g_display_ctx`, so the display can be updated only when necessary.</brief> |
| [task_display_minimal_pl](docs_src/task_display_minimal_pl.md) | <brief>Moduł `task_display_minimal` jest lekką implementacją UI/logowania dla `minimal-lwip`: minimalizuje obciążenie SSD1306, lustrzuje wiadomości do UART i pisze krótki ogon logu do `g_display_ctx`, aby wyświetlacz można było aktualizować tylko w razie potrzeby.</brief> |
| [task_display_minimal_ru](docs_src/task_display_minimal_ru.md) | Модуль `task_display_minimal` — облегчённая реализация UI/лога для `minimal-lwip`: он минимизирует нагрузку на SSD1306, зеркалит сообщения в UART и пишет короткий хвост в `g_display_ctx`, чтобы дисплей можно было обновлять только по необходимости. |
| [task_display_pl](docs_src/task_display_pl.md) | <brief>Moduł `task_display` zarządza stanem wizualnym portfela na SSD1306: odbiera zdarzenia sieciowe/podpisywania, łączy je w jeden wyświetlany stan i renderuje 4 linie UI z "przewijającym się tekstem" dla logów i danych sieciowych.</brief> |
| [task_display_ru](docs_src/task_display_ru.md) | Модуль `task_display` управляет визуальным состоянием кошелька на SSD1306: он принимает события сети/подписания, объединяет их в единое отображаемое состояние и рендерит 4 строки UI с "бегущей строкой" для логов и сетевых данных. |
| [task_io](docs_src/task_io.md) | <brief>The `task_io` module manages visual safety and status indicators: it periodically updates LED1 as "alive", manages LED2 as network indicator (depending on LwIP build), and enables LED3 when security alert is present.</brief> |
| [task_io_pl](docs_src/task_io_pl.md) | <brief>Moduł `task_io` zarządza wizualnymi wskaźnikami bezpieczeństwa i stanu systemu: okresowo aktualizuje LED1 jako "alive", zarządza LED2 jako wskaźnikiem sieciowym (w zależności od kompilacji LwIP) i włącza LED3 w przypadku security alert.</brief> |
| [task_io_ru](docs_src/task_io_ru.md) | Модуль `task_io` отвечает за визуальные индикаторы безопасности и статуса системы: он периодически обновляет LED1 как "alive", управляет LED2 как сетевым индикатором (в зависимости от сборки LwIP) и включает LED3 при наличии security alert. |
| [task_net](docs_src/task_net.md) | <brief>The `task_net` module is the network facade of the application: it brings up LwIP/Ethernet, starts an HTTP server on port 80, parses JSON/form POST requests (`POST /tx`), validates transactions, and sends them for signing via `g_tx_queue`.</brief> |
| [task_net_pl](docs_src/task_net_pl.md) | <brief>Moduł `task_net` jest fasadą sieciową aplikacji: podnosi LwIP/Ethernet, uruchamia serwer HTTP na porcie 80, parsuje żądania POST JSON/form (`POST /tx`), waliduje transakcje i wysyła je do podpisu poprzez `g_tx_queue`.</brief> |
| [task_net_ru](docs_src/task_net_ru.md) | `task_net` — это точка входа для всех внешних запросов подписания: хост (ПК, мобиль, веб-интерфейс) подключается по Ethernet, отправляет JSON или форму с адресом/суммой/валютой, а модуль парсит, валидирует, показывает на SSD1306 и ставит задачу в очередь для signing task. Без LwIP (`USE_LWIP=0`) модуль — no-op. Бизнес-роль — быть "врата в микроконтроллер" для сетевого клиента. |
| [task_user](docs_src/task_user.md) | <brief>The `task_user` module implements physical UX logic for the USER button (PC13): performs debounce, distinguishes short press (Confirm) from long hold (~2.5s) as Reject, and signals this to `task_sign` via `g_user_event_group`.</brief> |
| [task_user_pl](docs_src/task_user_pl.md) | <brief>Moduł `task_user` implementuje fizyczną logikę UX dla przycisku USER (PC13): wykonuje debounce, rozróżnia krótkie naciśnięcie (Confirm) od długiego przytrzymania (~2.5s) jako Reject i sygnalizuje to do `task_sign` poprzez `g_user_event_group`.</brief> |
| [task_user_ru](docs_src/task_user_ru.md) | Модуль `task_user` реализует физическую UX-логику для кнопки USER (PC13): делает debounce, различает короткое нажатие (Confirm) и длинное удержание (~2.5s) как Reject и сигналит это в `task_sign` через `g_user_event_group`. |
| [testing-plan-signing-rng](docs_src/testing-plan-signing-rng.md) | _Generated: 2026-03-19 22:45 UTC_ |
| [time_service](docs_src/time_service.md) | <brief>The `time_service` module provides time synchronization via SNTP and gives the application unified access to current Unix epoch and UTF string representation (for logs/UI), built on top of `HAL_GetTick()` after epoch received from network.</brief> |
| [time_service_pl](docs_src/time_service_pl.md) | <brief>Moduł `time_service` zapewnia synchronizację czasu poprzez SNTP i daje aplikacji zunifikowany dostęp do bieżącej epoki Uniksa i reprezentacji ciągu UTC (dla dzienników/UI), zbudowanej na bazie `HAL_GetTick()` po otrzymaniu epoki z sieci.</brief> |
| [time_service_ru](docs_src/time_service_ru.md) | Модуль `time_service` обеспечивает синхронизацию времени по SNTP и даёт приложению унифицированный доступ к текущему Unix epoch и строковому представлению UTC (для логов/UI), построенному поверх `HAL_GetTick()` после получения epoch из сети. |
| [usb_device](docs_src/usb_device.md) | <brief>The `usb_device` module brings up USB device for WebUSB: it configures USB clock source (HSI48), then starts USBD core, registers WebUSB class, and starts USB event handling.</brief> |
| [usb_device_pl](docs_src/usb_device_pl.md) | <brief>Moduł `usb_device` uruchamia urządzenie USB dla WebUSB: konfiguruje źródło zegara USB (HSI48), następnie uruchamia USBD core, rejestruje klasę WebUSB i uruchamia obsługę zdarzeń USB.</brief> |
| [usb_device_ru](docs_src/usb_device_ru.md) | Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий. |
| [usb_webusb](docs_src/usb_webusb.md) | <brief>The `usb_webusb` module implements vendor-specific WebUSB interface: it opens bulk endpoints, supports `ping` command (→ `pong`) and receives binary-framed signature request, extracts `recipient/amount/currency`, validates them and queues transaction to `g_tx_queue`; when signature ready, sends 64-byte compact `r\|\|s` via `WebUSB_NotifySignatureReady()`.</brief> |
| [usb_webusb_pl](docs_src/usb_webusb_pl.md) | <brief>Moduł `usb_webusb` implementuje interfejs WebUSB specyficzny dla vendora: otwiera bulk endpoints, obsługuje polecenie `ping` (→ `pong`) i odbiera binarnie oprawioną prośbę o podpis, wyodrębnia `recipient/amount/currency`, waliduje je i kolejkuje transakcję do `g_tx_queue`; gdy podpis jest gotowy, wysyła 64-bajtowy kompaktowy `r\|\|s` poprzez `WebUSB_NotifySignatureReady()`.</brief> |
| [usb_webusb_ru](docs_src/usb_webusb_ru.md) | Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`. |
| [usbd_conf](docs_src/usbd_conf.md) | <brief>Header `usbd_conf` is a thin wrapper: it includes `usbd_conf_cw.h`, thereby "binding" USB device middleware configuration to CryptoWallet's specific WebUSB BSP implementation.</brief> |
| [usbd_conf_cw](docs_src/usbd_conf_cw.md) | <brief>Module `usbd_conf_cw` is the BSP/configuration layer for USB device middleware: it provides a static allocator for USBD, describes memory hooks, configures MSP for PCD (GPIO alternate function, clock enable, NVIC priority/enable), and bridges HAL_PCD callbacks to USBD_LL events.</brief> |
| [usbd_conf_cw_pl](docs_src/usbd_conf_cw_pl.md) | <brief>Moduł `usbd_conf_cw` jest warstwą BSP/konfiguracyjną dla oprogramowania pośredniego USB device: zapewnia statyczny alokator dla USBD, opisuje hooki pamięci, konfiguruje MSP dla PCD (alternatywna funkcja GPIO, włączenie zegara, priorytet/włączenie NVIC) i łączy callback'i HAL_PCD ze zdarzeniami USBD_LL.</brief> |
| [usbd_conf_cw_ru](docs_src/usbd_conf_cw_ru.md) | <brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он предоставляет статический аллокатор для USBD, описывает memory hooks, настраивает MSP для PCD (GPIO alternate function, включение clock, NVIC priority/enable), и мостит HAL_PCD callback'ы в USBD_LL события.</brief> |
| [usbd_conf_pl](docs_src/usbd_conf_pl.md) | <brief>Nagłówek `usbd_conf` jest cienką opaką: zawiera `usbd_conf_cw.h`, tym samym "wiążąc" konfigurację oprogramowania pośredniego USB device do konkretnej implementacji WebUSB BSP CryptoWallet.</brief> |
| [usbd_conf_ru](docs_src/usbd_conf_ru.md) | <brief>Заголовок `usbd_conf` — тонкая обёртка: он включает `usbd_conf_cw.h`, тем самым "привязывая" конфигурацию USB device middleware к конкретной WebUSB BSP реализации CryptoWallet.</brief> |
| [usbd_desc_cw](docs_src/usbd_desc_cw.md) | <brief>Module `usbd_desc_cw` constructs USB descriptors for WebUSB device: device/interface/BOS and strings (manufacturer/product/serial), including WebUSB Platform Capability UUID, plus dynamic serial number generation based on STM32 DEVICE_ID registers.</brief> |
| [usbd_desc_cw_pl](docs_src/usbd_desc_cw_pl.md) | <brief>Moduł `usbd_desc_cw` konstruuje deskryptory USB dla urządzenia WebUSB: device/interface/BOS i ciągi (producent/produkt/serial), w tym UUID Platform Capability WebUSB, a także dynamiczna generacja numeru seryjnego na podstawie rejestrów STM32 DEVICE_ID.</brief> |
| [usbd_desc_cw_ru](docs_src/usbd_desc_cw_ru.md) | <brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief> |
| [wallet_shared](docs_src/wallet_shared.md) | <brief>The `wallet_shared` header defines a unified contract between modules: data structures for "request → confirmation → signature" and UI state, as well as global IPC handles (queues, event groups, mutexes) that are exchanged between `task_net`, `task_sign`, `task_user`, and `task_display`.</brief> |
| [wallet_shared_pl](docs_src/wallet_shared_pl.md) | <brief>Nagłówek `wallet_shared` definiuje zunifikowany kontrakt między modułami: struktury danych dla "żądanie → potwierdzenie → podpis" i stan UI, jak również globalne uchwyty IPC (kolejki, grupy zdarzeń, mutexy) które są wymieniane między `task_net`, `task_sign`, `task_user` i `task_display`.</brief> |
| [wallet_shared_ru](docs_src/wallet_shared_ru.md) | Заголовок `wallet_shared` задаёт единый контракт между модулями: структуры данных для "запрос → подтверждение → подпись" и UI-состояния, а также global IPC-ручки (очереди, event group, mutex'ы), которыми обмениваются `task_net`, `task_sign`, `task_user` и `task_display`. |
<!-- /DOXYGEN_DOCS_SRC_INDEX -->

## Project Structure

| Module | Brief |
|--------|-------|
| `Core/Inc/app_ethernet.h` | Ethernet link callback and DHCP thread interface. |
| `Core/Inc/crypto_wallet.h` | trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1. |
| `Core/Inc/hw_init.h` | Hardware initialization wrapper (CMSIS-compliant). |
| `Core/Inc/lwipopts.h` | (no description) |
| `Core/Inc/main.h` | Board pins, LEDs, UART logging macro, network IP defaults. |
| `Core/Inc/memzero.h` | Secure memory zeroing (prevents compiler optimization). |
| `Core/Inc/sha256_minimal.h` | Public API for minimal SHA-256 when trezor-crypto is disabled. |
| `Core/Inc/task_display.h` | OLED task API — SSD1306 UI types and Task_Display_Log . |
| `Core/Inc/task_io.h` | IO module - LEDs only. |
| `Core/Inc/task_net.h` | LwIP + HTTP — Ethernet stack and port 80 API for transactions. |
| `Core/Inc/task_security.h` | Header for legacy task_security.c (mock crypto FSM). |
| `Core/Inc/task_sign.h` | Production signing task — g_tx_queue consumer, USER confirm, ECDSA. |
| `Core/Inc/task_user.h` | User button task — USER key (PC13) handling. |
| `Core/Inc/time_service.h` | SNTP API — init, start after link, epoch + formatted UTC. |
| `Core/Inc/tx_request_validate.h` | Request analysis and validation for crypto transaction signing. |
| `Core/Inc/usb_device.h` | USB device init for CryptoWallet WebUSB. |
| `Core/Inc/usb_webusb.h` | WebUSB vendor-specific class for CryptoWallet. |
| `Core/Inc/usbd_conf.h` | USB device conf - redirects to CryptoWallet WebUSB config. |
| `Core/Inc/usbd_conf_cw.h` | USB device BSP configuration for CryptoWallet WebUSB. |
| `Core/Inc/usbd_desc_cw.h` | USB device descriptors for CryptoWallet WebUSB. |
| `Core/Inc/wallet_shared.h` | Shared types and IPC: queues, events, mutexes, display context. |
| `Core/Src/crypto_wallet.c` | trezor-crypto glue: STM32 TRNG, random_buffer , BIP-32, ECDSA sign. |
| `Core/Src/hw_init.c` | Board bring-up: clock, MPU/cache, GPIO, I2C1 (OLED), UART, optional USB. |
| `Core/Src/main.c` | FreeRTOS entry: IPC objects, task creation, OS hooks. |
| `Core/Src/memzero.c` | Secure memzero() — volatile byte writes (no optimize-out). |
| `Core/Src/sha256_minimal.c` | SHA-256 only — used when USE_CRYPTO_SIGN=0 (no trezor-crypto). |
| `Core/Src/stm32h7xx_hal_msp.c` | MSP init for CryptoWallet - I2C1 (SSD1306). |
| `Core/Src/stm32h7xx_it.c` | Interrupt handlers - FreeRTOS SysTick, ETH (when USE_LWIP). |
| `Core/Src/stm32h7xx_it_systick.c` | SysTick handler for minimal-lwip (FreeRTOS tick). |
| `Core/Src/stm32h7xx_it_usb.c` | USB OTG HS interrupt handler (WebUSB). |
| `Core/Src/task_display.c` | SSD1306 128×32 — four scroll lines, state machine, queue-driven UI. |
| `Core/Src/task_display_minimal.c` | Reduced display task for minimal-lwip — faster Ethernet-first bring-up. |
| `Core/Src/task_io.c` | LED policy task — system / network / alert indicators only. |
| `Core/Src/task_security.c` | Alternate signing FSM with mock SHA256/ECDSA (placeholders). |
| `Core/Src/task_sign.c` | Primary signing task — consumes g_tx_queue , USER confirm, ECDSA. |
| `Core/Src/task_user.c` | Physical UX — USER (PC13) debounce, confirm vs reject for signing. |
| `Core/Src/time_service.c` | SNTP client — wall-clock epoch and UTC strings for logs/UI. |
| `Core/Src/tx_request_validate.c` | Validate host-supplied recipient / amount / currency before signing. |
| `Core/Src/usb_device.c` | USB device initialization for CryptoWallet WebUSB. |
| `Core/Src/usb_webusb.c` | WebUSB vendor class — ping/pong and binary sign request/response. |
| `Core/Src/usbd_conf_cw.c` | USB device BSP for CryptoWallet WebUSB (NUCLEO-H743ZI2, PA11/PA12). |
| `Core/Src/usbd_desc_cw.c` | USB device descriptors for CryptoWallet WebUSB. |
| `Core/Src/wallet_seed.c` | Strong get_wallet_seed() when USE_TEST_SEED=1 (development only). |
| `Drivers/ssd1306/ssd1306_conf.h` | Display driver tuning — I2C1, 128×32, 0x3C, Font 6×8. |
| `Src/app_ethernet_cw.c` | Ethernet glue — link callbacks, DHCP state machine, LED feedback. |
| `Src/task_net.c` | LwIP + HTTP — DHCP/static IP, POST /tx, signing poll endpoints. |
