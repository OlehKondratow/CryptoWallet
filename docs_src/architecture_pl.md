\page architecture "CryptoWallet: Architektura i projektowanie systemu"

# Przegląd architektury CryptoWallet

## Streszczenie projektu

**CryptoWallet** to modularna firmware do podpisywania transakcji Bitcoin na mikrokontrolerze **STM32H743ZI2** (Nucleo-H743ZI2) z użyciem **FreeRTOS**. Integruje operacje kryptograficzne (BIP-39, BIP-32, ECDSA secp256k1), bezpieczeństwo sprzętu (MPU, TRNG), funkcje sieciowe (LwIP, Ethernet, WebUSB) i interfejsy użytkownika (wyświetlacz OLED, przyciski).

## Architektura wysokiego poziomu

System podąża **architekturze opartej na zadaniach i zdarzeniach**:

```
┌─────────────────────────────────────────────────────────┐
│  STM32H743ZI2 (ARM Cortex-M7, 480 MHz, 2 MB Flash)    │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────┐   │
│  │  Harmonogram FreeRTOS (CMSIS-RTOS2)             │   │
│  ├──────────────────────────────────────────────────┤   │
│  │ Zadanie 1: hw_init         (Inicjalizacja)      │   │
│  │ Zadanie 2: task_display    (OLED UI, SSD1306)   │   │
│  │ Zadanie 3: task_io         (Kontrola LED/Btn)   │   │
│  │ Zadanie 4: task_net        (Sieć, LwIP)         │   │
│  │ Zadanie 5: task_sign       (Podpis kryptogr.)   │   │
│  │ Zadanie 6: task_security   (Autentykacja)       │   │
│  │ Zadanie 7: usb_device      (WebUSB, CDC)        │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Kolejki, grupy zdarzeń, muteksy (IPC)         │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  HAL / Sterowniki (GPIO, I2C, UART, Eth, USB)  │   │
│  ├──────────────────────────────────────────────────┤   │
│  │  Stos LwIP TCP/IP    │  trezor-crypto           │   │
│  │  (DHCP, SNTP, DNS)   │  (BIP39/32, ECDSA, SHA)  │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
     │         │         │         │         │
     ▼         ▼         ▼         ▼         ▼
  ┌─────┐ ┌─────┐ ┌──────┐ ┌────────┐ ┌────────┐
  │OLED │ │LED/ │ │Ether-│ │ WebUSB │ │ UART   │
  │(I2C)│ │BTN  │ │ net  │ │(USB-HS)│ │(Debug) │
  └─────┘ └─────┘ └──────┘ └────────┘ └────────┘
```

## Główne komponenty

### 1. Inicjalizacja (`main.c` / `hw_init.c`)

**Odpowiedzialność:**
- Konfiguracja MPU (Memory Protection Unit) **przed** HAL_Init (wymagana dla bezpieczeństwa DMA LwIP)
- Włączenie/wyłączenie cache w zależności od regionów pamięci
- Inicjalizacja drzewa zegarów (PLL, dystrybucja clock)
- Konfiguracja NVIC dla priorytetów przerwań
- Inicjalizacja TRNG dla entropii kryptograficznej
- Gejting zegara urządzeń peryferyjnych i konfiguracja GPIO
- Uruchomienie harmonogramu FreeRTOS

**Kluczowe flagi:**
- `USE_LWIP` — Włącz Ethernet + stos LwIP
- `USE_WEBUSB` — Włącz tryb USB WebUSB
- `USE_CRYPTO_SIGN` — Włącz pełny ECDSA/BIP-39/BIP-32 (vs. tylko SHA-256)
- `USE_TEST_SEED` — Użyj hardcoded ziarna testowego (tylko opracowanie)
- `BOOT_TEST` — Tryb diagnostyczny bez FreeRTOS (uart debug loop)

### 2. Komunikacja między zadaniami (IPC)

**Globalne obiekty synchronizacji:**

```c
// Kolejki
QueueHandle_t g_tx_queue;          // task_net → task_sign (wallet_tx_t)
QueueHandle_t g_display_queue;     // task_net → task_display (Transaction_Data_t)

// Grupy zdarzeń
EventGroupHandle_t g_user_event_group; // task_io → task_sign (potwierdzenie użytkownika)

// Muteksy
SemaphoreHandle_t g_i2c_mutex;         // Arbitraż szyny I2C1
SemaphoreHandle_t g_display_ctx_mutex; // Ochrona kontekstu wyświetlacza
```

**Przepływ danych:**
1. Pakiet sieciowy (HTTP POST `/tx` lub WebUSB endpoint)
2. Analiza w `task_net` → wysłanie do `g_tx_queue`
3. `task_sign` odbiera, wyświetla transakcję na OLED
4. Użytkownik naciska przycisk → ustawia zdarzenie w `g_user_event_group`
5. `task_sign` wyprowadza klucz, podpisuje transakcję, wysyła odpowiedź

### 3. Zadanie wyświetlacza (`task_display.c`)

**Automat stanów skończonych (FSM):**

| Stan | Tryb wyświetlacza | Cel |
|---|---|---|
| `DISPLAY_WALLET` | Kryptowaluta + kwota | Pokaż szczegóły transakcji |
| `DISPLAY_SECURITY` | Status blokady + podpis | Potwierdzenie autentykacji/podpisu |
| `DISPLAY_NETWORK` | IP/MAC + status USB | Informacje o łączności sieciowej |
| `DISPLAY_LOG` | Przewijany dziennik systemu | Debugowanie/diagnostyka |

**Sprzęt:** SSD1306 OLED (128×32, I2C1, addr 0x3C, PB8/PB9)

### 4. Integracja kryptograficzna (`crypto_wallet.c` / `task_sign.c`)

**Potok podpisywania:**

```
Wejście użytkownika (HTTP POST /tx lub WebUSB)
    ↓
Walidacja (format adresu, kwota, whitelist walut)
    ↓
build_hash_input() → "recipient|amount|currency"
    ↓
crypto_hash_sha256() → 32-bajtowy skrót
    ↓
Potwierdzenie użytkownika (naciśnięcie przycisku na OLED)
    ↓
get_wallet_seed() → 64-bajtowe ziarno BIP-39
    ↓
crypto_derive_btc_m44_0_0_0_0() → klucz prywatny (32 bajty)
    ↓
crypto_sign_btc_hash() → podpis ECDSA (64 bajty: r||s)
    ↓
memzero() → Wyczyść poufne bufory
    ↓
Odpowiedź (USB WebUSB lub HTTP JSON)
```

**Funkcje kryptograficzne (poprzez trezor-crypto):**
- **BIP-39:** Generacja mnemoniki ze 128-bitowej entropii
- **BIP-32:** Pochodne klucze HD wzdłuż ścieżki `m/44'/0'/0'/0/0`
- **ECDSA:** Podpis secp256k1 z deterministycznym RFC6979 k
- **SHA-256:** Haszowanie skrótu transakcji
- **Entropia:** STM32 TRNG + pula programowego LCG

**Kompilacja warunkowa:**
- `USE_CRYPTO_SIGN=1` → Pełny ECDSA, ~100-150 KB binarki
- `USE_CRYPTO_SIGN=0` → Tylko SHA-256, ~5 KB binarki
- `USE_TEST_SEED=1` → Powtarzalne ziarno testowe (bez bezpieczeństwa)

### 5. Funkcjonalność sieciowa (`task_net.c` / `app_ethernet_cw.c`)

**Integracja LwIP:**
- Stos IPv4 TCP/IP z klientem DHCP
- Rezerwowy statyczny IP: `192.168.0.10`
- Synchronizacja SNTP (aktualizacja poprzez `time_service_set_epoch()`)
- Serwer HTTP dla endpoint'a POST `/tx`
- Opcjonalny WebUSB poprzez `usb_device.c`

**Ethernet PHY:** Zintegrowany w STM32H743, bufory DMA w D2 SRAM (cache musi być wyłączony)

**Automat stanów:**
1. Wykrycie łącza (przerwanie PHY)
2. Żądanie DHCP
3. IP otrzymany
4. Żądanie czasu SNTP
5. Gotów do transakcji

### 6. I/O i interfejs użytkownika (`task_io.c`)

**Mapowanie GPIO:**

| Funkcja | Pin | Tryb |
|---|---|---|
| LED zielony | PB0 | Wyjście |
| LED żółty | PE1 | Wyjście |
| LED czerwony | PE2 | Wyjście |
| Przycisk użytkownika | PC13 | Wejście (EXTI) |

**Antydrebezg:** Sprzętowy EXTI z programowym filtrem drebezgu (20 ms)

### 7. USB i WebUSB (`usb_device.c` / `usb_webusb.c`)

**Konfiguracja WebUSB:**
- VID/PID: `0x1209 / 0xC0DE`
- Szybkość USB: Full-speed (USB 1.1, 12 Mbps)
- Deskryptor BOS z UUID Platform Capability WebUSB
- Numer seryjny dynamicznie generowany z rejestrów STM32 DEVICE_ID

**Endpoint'y:**
- EP0: Control (domyślny)
- EP1: Data-out (hosta → urządzenie)
- EP2: Data-in (urządzenie → hosta)

### 8. HAL — abstrakcja mikrokontrolera

**Pliki MSP (Microcontroller Support Package):**
- `stm32h7xx_hal_msp.c` — Konfiguracja GPIO/clock/NVIC dla urządzeń peryferyjnych
- `stm32h7xx_it.c` — Procedury obsługi przerwań (SysTick, Ethernet IRQ)
- `stm32h7xx_it_usb.c` — Procedura obsługi OTG_HS IRQ dla WebUSB
- `stm32h7xx_it_systick.c` — Alternatywny SysTick dla minimalnych kompilacji LwIP

## Architektura bezpieczeństwa

### Memory Protection Unit (MPU)

- **Region 0:** Flash (kod, read-only)
- **Region 1:** D1 SRAM (dane ogólne)
- **Region 2:** D2 SRAM (stos LwIP, cache wyłączony dla bezpieczeństwa DMA)
- **Region 3:** Peryferyjny I/O (device memory)

**Ustawienia cache:**
- D-Cache: Wyłączony dla D2 (region Ethernet DMA)
- I-Cache: Włączony dla flash (pobieranie instrukcji)

### Generacja liczb losowych

**Mieszanie entropii:**
1. Sprzętowy TRNG STM32H743 (`HAL_RNG_GenerateRandomNumber()`)
2. Pula entropii programowego LCG (parametry Numerical Recipes)
3. Kombinacja XOR dla wybielenia

### Obsługa materiału klucza

- Ziarno BIP-39 obliczane na żądanie, nigdy nie przechowywane
- Klucze prywatne przydzielone na stosie z ograniczonym czasem życia
- `memzero()` czyści wszystkie poufne bufory przed powrotem funkcji
- Brak globalnego przechowywania klucza w flash firmware

### Bezpieczne funkcje

- Deterministyczne ECDSA (RFC6979) zapobiega atakom ponownego użycia k
- Walidacja Base58Check dla integralności adresu Bitcoin
- Sanityzacja wejścia (regex, whitelist) dla żądań transakcji

## Kompilacja i system budowania

### Cele Makefile

```bash
make              # Zbuduj firmware
make clean        # Oczyść artefakty budowania
make docs-doxygen # Wygeneruj dokumentację API (HTML/XML)
make flash        # Zaprogramuj STM32 poprzez JTAG/SWD
make monitor      # Konsola debugowania UART
```

### Konfiguracja budowania

```bash
# Przykład: pełna kompilacja funkcji
make USE_LWIP=1 USE_WEBUSB=1 USE_CRYPTO_SIGN=1

# Minimalna kompilacja (tylko SHA-256)
make USE_CRYPTO_SIGN=0

# Tryb testu
make BOOT_TEST=1 USE_TEST_SEED=1
```

## Statystyka projektu

| Metryka | Wartość |
|---|---|
| **Całkowitych plików** | ~50 plików C/C++ |
| **Linie kodu** | ~15,000+ |
| **Główne moduły** | 14 (main, display, IO, network, sign, security, USB, crypto itp.) |
| **Dokumentacja** | 43 pliki markdown (EN/RU/PL) |
| **Rozmiar binarki** | ~500 KB (pełna kompilacja z kryptem) |
| **Użycie SRAM** | ~100 KB (stosy FreeRTOS, kolejki) |
| **Rozmiar projektu** | ~50 MB (wkl. trezor-crypto) |

## Zależności

### Sprzęt

- **MCU:** STM32H743ZI2 (ARM Cortex-M7, 480 MHz, 2 MB Flash, 512 KB SRAM)
- **Płyta:** Nucleo-H743ZI2
- **Wyświetlacz:** SSD1306 OLED 128×32 (I2C1)
- **Sieć:** Zintegrowany Ethernet PHY + RJ45

### Oprogramowanie

- **STM32 HAL:** Biblioteka abstrakcji sprzętu
- **FreeRTOS:** API CMSIS-RTOS2
- **LwIP:** Lekki stos TCP/IP
- **trezor-crypto:** Biblioteka kryptografii Bitcoin (MIT License)
- **SSD1306 Driver:** Projekt stm32-ssd1306

### Projekty zewnętrzne

- `/data/projects/STM32CubeH7` — Sterowniki HAL
- `/data/projects/stm32-ssd1306` — Biblioteka sterownika OLED
- `/data/projects/stm32_secure_boot` — Skrypty Linker, konfiguracja FreeRTOS
- `/data/projects/trezor-firmware` — Referencyjna implementacja trezor-crypto

## Struktura dokumentacji

- **Architektura:** Ten plik (projektowanie systemu wysokiego poziomu)
- **Dokumentacja API:** Wygenerowana z Doxygen z komentarzy źródłowych
- **Dokumentacja modułów:** `docs_src/*.md` (przepływ logiki szczegółowy na plik)
- **Integracja trezor-crypto:** `docs_src/trezor-crypto-integration.md`
- **Konfiguracja sprzętu:** `docs_src/hw_init.md`, `docs_src/lwipopts.md`

## Dalsze kroki i przyszłe ulepszenia

1. **Integracja elementu bezpiecznego:** Przenieś magazyn kluczy do SE (np. ATECC608)
2. **Wsparcie dodatkowych monet:** Rozszerz ścieżki BIP-44 dla Ethereum, Litecoin
3. **Bootloader:** Zaimplementuj bezpieczny mechanizm aktualizacji firmware
4. **Formalna weryfikacja:** Audyt bezpieczeństwa + dowody poprawności kryptografii
5. **Hartowanie produkcji:** Zgodność FIPS 140-2, odporność na kanały boczne
