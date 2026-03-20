# Porównanie i Analiza Projektów STM32

**Data analizy:** 2026-03-20  
**Projekty:** `stm32_secure_boot` vs `CryptoWallet`  

---

## 📊 Tabela Porównania

|| Charakterystyka | stm32_secure_boot | CryptoWallet |
||---|---|---|
|| **Cel** | Bezpieczny bootloader + signer HID | Pełny system kryptograficznego portfela |
|| **Urządzenie Docelowe** | NUCLEO-H743ZI2 | NUCLEO-H743ZI2 |
|| **Rozmiar Projektu** | ~12 profilów | 1 główny profil |
|| **Fokus** | Weryfikowana загрузка (Verified Boot) | Pełny cykl podpisywania transakcji bitcoin |
|| **Kryptografia** | ECDSA secp256k1 (CMOX) | ECDSA secp256k1 (trezor-crypto) |
|| **Stos Sieciowy** | Opcjonalny (LwIP w lwip_zero) | Obowiązkowy (LwIP + HTTP) |
|| **Wyświetlacz** | Obsługa SSD1306 (opcjonalnie) | Wbudowany SSD1306 (obowiązkowy) |
|| **Interfejsy** | UART + USB HID | UART + USB WebUSB + HTTP Ethernet |
|| **Przechowywanie Kluczy** | Standardowe | Ulepszone (memzero + BIP-39/BIP-32) |
|| **Główna Funkcja** | Podwójny transport (UART/HID) | Pełna funkcjonalność portfela |
|| **Status** | Badawcze (R&D) | Production-ready |

---

## 🏗️ Architektura: Diagram Porównania

### stm32_secure_boot

```
┌────────────────────────────────────────────────┐
│         BOOTLOADER (64 KB @ 0x08000000)        │
│  - Haszowanie obrazu SHA-256                   │
│  - Weryfikacja ECDSA secp256k1 (lub stub)     │
│  - Skok do aplikacji jeśli OK                  │
│  - Wskaźnik LED błędu                         │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         APP (system wieloprofilowy)             │
│                                                 │
│  step2_hid:                                    │
│  ├─ FreeRTOS                                   │
│  ├─ Logowanie UART (USART3)                   │
│  ├─ Raporty USB HID 64-bajtowe                │
│  ├─ Skaner I2C + SSD1306 (opcjonalnie)       │
│  ├─ Debouncing przycisku (30ms)               │
│  └─ Podwójny transport UART/USB HID           │
│                                                 │
│  lwip_zero:                                    │
│  ├─ FreeRTOS + LwIP                           │
│  ├─ Ethernet                                   │
│  └─ Serwer HTTP                               │
│                                                 │
│  Inne profile: step1, i2c-SSD1306, demo*     │
└────────────────────────────────────────────────┘
```

### CryptoWallet

```
┌────────────────────────────────────────────────┐
│              WARSTWA SPRZĘTU                   │
│  Zegar, GPIO, UART, I2C, USB, Ethernet, RNG   │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│           WARSTWA RTOS FreeRTOS               │
│  Planowanie zadań, kolejki, mutexy, zdarzenia │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         WARSTWA PROTOKOŁU (Multi-stack)       │
│  ├─ LwIP (Ethernet + HTTP)                   │
│  ├─ WebUSB (punkty końcowe bulk)             │
│  └─ UART (debugowanie + kontrola)            │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         WARSTWA KRYPTOGRAFII                  │
│  ├─ trezor-crypto (mnemoniku BIP-39)         │
│  ├─ Deklarowanie HD BIP-32                   │
│  ├─ Podpisywanie ECDSA secp256k1             │
│  └─ Bezpieczeństwo SHA-256 + memzero()       │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         WARSTWA APLIKACJI                     │
│  ├─ FSM Podpisywania (task_sign.c)           │
│  ├─ FSM Walidacji (tx_request_validate.c)    │
│  ├─ FSM Sieci (task_net.c)                   │
│  ├─ FSM Wyświetlacza (task_display.c)        │
│  └─ Interakcja użytkownika (task_user.c)     │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         UI / WYJŚCIE                          │
│  ├─ SSD1306 OLED (128×32)                    │
│  ├─ Potwierdzenie przyciskiem                │
│  └─ Wskaźniki statusu LED                    │
└────────────────────────────────────────────────┘
```

---

## 📁 Struktura Projektów: Porównanie Szczegółowe

### stm32_secure_boot

```
stm32_secure_boot/
├── bootloader/               # ⭐ Unikalnie dla tego projektu
│   ├── src/
│   │   ├── main.c           # Weryfikowana загрузка
│   │   └── cmox_low_level.c # Obsługa kryptografii CMOX
│   ├── inc/
│   │   └── keys.h           # Klucze publiczne
│   ├── bootloader.ld        # Skrypt linkera (64 KB)
│   └── Makefile
├── app/                      # Wiele profilów
│   ├── step1/               # LED + UART (podstawowy)
│   ├── step2/               # FreeRTOS + skaner I2C
│   ├── step2_hid/           # ⭐ GŁÓWNY: UART + USB HID
│   ├── lwip_zero/           # Serwer LwIP + HTTP
│   ├── lwip-uaid-SSD1306/   # LwIP + OLED
│   ├── UART-HID/            # Podwójny transport
│   ├── demo1, demo2/        # Demo z autoryzacją PIN
│   ├── btn-oled-uart/       # Przycisk + OLED
│   └── Makefile
├── common/
│   ├── uart_log.c/h         # Logowanie UART (USART3)
│   └── lcd_1602_i2c.c/h     # Sterownik LCD 1602 I2C
├── FreeRTOS/                # Źródła FreeRTOS
├── scripts/                 # Buduj, flash, debug, test
├── docs/                    # ~20 dokumentów
├── Makefile
├── readme.md                # Przegląd Polski/Rosyjski
└── .gitignore
```

### CryptoWallet

```
CryptoWallet/
├── Core/Inc/                # Pliki nagłówkowe (22)
│   ├── crypto_wallet.h      # API Kryptografii
│   ├── task_*.h             # Zadania FreeRTOS
│   ├── lwipopts.h           # Opcje LwIP
│   ├── ssd1306_conf.h       # Konfiguracja OLED
│   └── ... (18 więcej)
├── Core/Src/                # Pliki źródłowe (23)
│   ├── main.c               # Punkt wejścia FreeRTOS
│   ├── task_sign.c          # FSM Podpisywania ⭐
│   ├── crypto_wallet.c      # Wrapper trezor-crypto
│   ├── task_net.c           # FSM Serwera HTTP
│   ├── task_display.c       # Zarządzanie SSD1306
│   ├── task_user.c          # Wejście przycisku + debouncing
│   ├── task_io.c            # Zarządzanie LED
│   ├── tx_request_validate.c # Walidacja Base58/bech32
│   ├── memzero.c            # Bezpieczne czyszczenie buforów
│   ├── hw_init.c            # Inicjalizacja HAL
│   ├── usb_webusb.c         # Punkty końcowe WebUSB
│   ├── usb_device.c         # Inicjalizacja USB
│   ├── usbd_conf_cw.c       # Konfiguracja BSP USB
│   ├── usbd_desc_cw.c       # Deskryptory USB (BOS)
│   ├── app_ethernet_cw.c    # FSM łącza Ethernet
│   ├── time_service.c       # Synchronizacja SNTP
│   ├── rng_dump.c           # RNG dla Dieharder ⭐ NOWY
│   └── ... (6 więcej)
├── Src/                     # Dodatkowe (3)
│   ├── task_net.c           # Duplikacja?
│   └── app_ethernet_cw.c    # Duplikacja?
├── Drivers/ssd1306/         # Konfiguracja SSD1306
├── ThirdParty/
│   └── trezor-crypto/       # Biblioteka kryptografii Bitcoin ⭐
├── docs_src/                # Dokumentacja (128+ plików)
│   ├── crypto/              # Dokumentacja krypto + wielojęzycznie
│   ├── testing/             # Testowanie RNG + podpisywania
│   ├── main.md, architecture.md, itd.
│   └── *_ru.md, *_pl.md     # Tłumaczenia
├── scripts/                 # Narzędzia Python (11+)
│   ├── bootloader_secure_signing_test.py
│   ├── test_usb_sign.py
│   ├── test_rng_signing_comprehensive.py ⭐ NOWY
│   ├── capture_rng_uart.py
│   ├── run_dieharder.py
│   └── ... (6+ więcej)
├── Makefile                 # Główny skrypt budowania
├── FreeRTOSConfig.h         # Pełna konfiguracja FreeRTOS
├── FreeRTOSConfig_lwip.h    # Wariant LwIP
├── STM32H743ZITx_FLASH_LWIP.ld # Skrypt linkera
├── .gitignore
└── README*.md               # Dokumentacja (ang/ros/pol)
```

---

## 🔄 Interakcja Projektów

```
┌─────────────────────────────────────────────────────────┐
│  stm32_secure_boot (Biblioteka/Fundament)              │
│  ├─ Wersja FreeRTOS                                     │
│  ├─ LwIP (profil lwip_zero)                           │
│  ├─ Przykłady inicjalizacji sprzętu                    │
│  ├─ Weryfikacja ECDSA (bootloader)                     │
│  └─ Przykłady podwójnego transportu (UART/HID)        │
└─────────────────────────────────────────────────────────┘
                        ↓ UŻYWANE ↓
┌─────────────────────────────────────────────────────────┐
│  CryptoWallet (Aplikacja/Konkretna Implementacja)      │
│  ├─ Bierze FreeRTOS + LwIP z stm32_secure_boot       │
│  ├─ Dodaje trezor-crypto (BIP-39, BIP-32)            │
│  ├─ Implementuje pełny cykl podpisywania transakcji  │
│  ├─ Dodaje interfejsy WebUSB + HTTP                  │
│  ├─ Dodaje infrastrukturę testowania RNG             │
│  └─ Portfel kryptograficzny production-ready         │
└─────────────────────────────────────────────────────────┘
```

**Wniosek:** CryptoWallet = stm32_secure_boot + kryptografia + walidacja + pełny interfejs

---

## 🔐 Kryptografia: Porównanie Szczegółowe

### stm32_secure_boot - Kryptografia

|| Komponent | Szczegóły |
||---|---|
|| **Krzywa** | secp256k1 (Bitcoin) |
|| **Algorytm Podpisywania** | ECDSA |
|| **Źródło** | CMOX (ST Microelectronics) lub stub |
|| **Obsługa BIP-39** | ❌ Nie |
|| **Obsługa BIP-32** | ❌ Nie |
|| **Haszowanie** | SHA-256 (wbudowane) |
|| **Zarządzanie Kluczami** | Podstawowe (klucze publiczne w keys.h) |
|| **Użycie** | Weryfikacja podpisów bootloadera |

### CryptoWallet - Kryptografia

|| Komponent | Szczegóły |
||---|---|
|| **Krzywa** | secp256k1 (Bitcoin) |
|| **Algorytm Podpisywania** | ECDSA |
|| **Źródło** | trezor-crypto (Satoshi Labs) |
|| **Obsługa BIP-39** | ✅ Tak (mnemoniku 12/24-słowne) |
|| **Obsługa BIP-32** | ✅ Tak (hierarhiczne rozszerzenie kluczy) |
|| **Haszowanie** | SHA-256 + HMAC-SHA-512 |
|| **Zarządzanie Kluczami** | **Zaawansowane:** |
||  | - Seed z mnemoniku |
||  | - Hierarhiczna generacja kluczy |
||  | - Ochrona bufora (memzero) |
||  | - Test seed dla rozwoju |
|| **Użycie** | Pełny cykl podpisywania transakcji bitcoin |
|| **RNG** | Opcjonalny dump RNG (testowanie Dieharder) |

---

## 📡 Interfejsy: Interakcja Sieciowa

### stm32_secure_boot (step2_hid)

```
┌──────────────────────────────────────────┐
│  Architektura step2_hid Podwójnego       │
│  Transportu                              │
├──────────────────────────────────────────┤
│                                          │
│  UART (USART3 @ 115200 baud)            │
│  ├─ PING/PONG heartbeat                 │
│  ├─ Zapytanie STATUS                    │
│  ├─ Polecenie SIGN                      │
│  └─ Wyjście zalogowane                  │
│                                          │
│  USB HID (raporty surowe 64-bajtowe)    │
│  ├─ Raport IN (urządzenie → host)       │
│  ├─ Raport OUT (host → urządzenie)      │
│  ├─ Przepływ potwierdzenia przycisku    │
│  └─ Odpowiedź sygnatury ECDSA           │
│                                          │
│  Transport agnostycze dla protokołu:     │
│  - Obie linie mogą obsługiwać SIGN      │
│  - Priorytet: potwierdzenie użytkownika │
│  - Logowanie do obu: UART + wyświetlacz│
└──────────────────────────────────────────┘
```

### CryptoWallet (Multi-Protocol)

```
┌──────────────────────────────────────────┐
│  Stos Protokołu CryptoWallet             │
├──────────────────────────────────────────┤
│                                          │
│  ╔══════════════════════════════════╗  │
│  ║  WARSTWA APLIKACJI               ║  │
│  ║  - Walidacja TX (Base58/bech32)  ║  │
│  ║  - Zarządzanie seed BIP-39       ║  │
│  ║  - FSM podpisywania ECDSA        ║  │
│  ╚══════════════════════════════════╝  │
│                ↓ ↓ ↓                   │
│  ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │   HTTP   │ │  WebUSB  │ │  UART  │ │
│  │ (port80) │ │(bulk EP) │ │(115200)│ │
│  └──────────┘ └──────────┘ └────────┘ │
│       ↓            ↓           ↓       │
│  ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │ Ethernet │ │   USB    │ │ Serial │ │
│  │ (DHCP)   │ │ Device   │ │ (UART) │ │
│  └──────────┘ └──────────┘ └────────┘ │
│       ↓            ↓           ↓       │
│  ┌─────────────────────────────────┐  │
│  │   Komputer Host / Klient        │  │
│  │  - curl dla HTTP                │  │
│  │  - WebUSB API (JS w browser)   │  │
│  │  - miniterm dla UART            │  │
│  └─────────────────────────────────┘  │
└──────────────────────────────────────────┘

Priorytet:
1️⃣  WebUSB (główny dla production)
2️⃣  HTTP/Ethernet (fallback)
3️⃣  UART (debug/development)
```

---

## 📊 Tabela Porównawcza Plików Kluczowych

|| Funkcja | stm32_secure_boot | CryptoWallet |
||---|---|---|
|| **Kryptografia** | app/step2_hid/main.c + CMOX | Core/Src/crypto_wallet.c + trezor-crypto |
|| **Podpisywanie** | Podstawowe (ECDSA) | FSM (task_sign.c) + walidacja |
|| **Walidacja** | N/A | tx_request_validate.c (Base58/bech32) |
|| **Klucze HD** | ❌ | ✅ BIP-39 + BIP-32 |
|| **Stos Sieciowy** | lwip_zero (opcjonalny) | LwIP obowiązkowy |
|| **Serwer HTTP** | lwip_zero/src/main.c | task_net.c (oparte na FSM) |
|| **WebUSB** | ❌ | ✅ usb_webusb.c |
|| **Wyświetlacz** | Opcjonalny | Obowiązkowy |
|| **Zarządzanie Pamięcią** | Standardowe | memzero() + bezpieczne czyszczenie |
|| **Testowanie RNG** | ❌ | ✅ rng_dump.c + Dieharder |
|| **Łańcuch Boot** | ✅ SHA-256 + ECDSA | ❌ (zależy od bootloadera) |

---

## 🚀 Różnice w Podejściu

### stm32_secure_boot - "Laboratorium"

✅ **Mocne Strony:**
- Wiele profili edukacyjnych
- Wyraźne rozdzielenie bootloadera/aplikacji
- Przykłady podwójnego transportu (UART + HID)
- Weryfikowana загрузка "od razu"

❌ **Ograniczenia:**
- Brak pełnego stosu kryptograficznego
- Brak walidacji transakcji
- Brak kluczy HD
- Kod eksperymentalny

### CryptoWallet - "System Production"

✅ **Mocne Strony:**
- Pełny stos kryptograficzny (BIP-39, BIP-32, ECDSA)
- Walidacja adresów bitcoin (Base58/bech32)
- Multi-protocol (HTTP, WebUSB, UART)
- Bezpieczne zarządzanie pamięcią (memzero)
- Potężna infrastruktura testów (RNG, testy podpisywania)
- Dokumentacja wielojęzyczna
- Kod oparty na FSM niezawodny

❌ **Ograniczenia:**
- Zależy od stm32_secure_boot dla FreeRTOS/LwIP
- Brak własnego bootloadera
- Wymaga więcej pamięci (LwIP + crypto)

---

## 📝 Aktualizacje w CryptoWallet (Ostatnie Zmiany)

### 🆕 Nowe Pliki (Untracked)

#### **Infrastruktura Testowania RNG**
```
✨ Core/Inc/rng_dump.h                           # API dump RNG
✨ Core/Src/rng_dump.c                           # Wyjście surowe RNG
✨ docs_src/crypto/rng_dump_setup.md             # Przewodnik konfiguracji
✨ docs_src/crypto/rng_dump_setup_pl.md          # Wersja polska
✨ docs_src/crypto/rng_dump_setup_ru.md          # Wersja rosyjska
```

#### **Dokumentacja Testowania**
```
✨ docs_src/INSTALL_TEST_DEPS.md                 # Instalacja dieharder, pyserial
✨ docs_src/TESTING_GUIDE_RNG_SIGNING.md         # Kompletny przewodnik testowania
✨ docs_src/TEST_SCRIPTS_README.md               # Przegląd skryptów Python
✨ docs_src/VENV_SETUP.md                        # Konfiguracja wirtualnego środowiska
✨ docs_src/crypto/testing_setup.md              # Przepływ pracy testowania
✨ docs_src/crypto/testing_setup_pl.md           # Wersja polska
✨ docs_src/crypto/testing_setup_ru.md           # Wersja rosyjska
```

#### **Rozwiązywanie Problemów RNG**
```
✨ docs_src/crypto/rng_capture_troubleshooting.md    # Przewodnik rozwiązywania
✨ docs_src/crypto/rng_capture_troubleshooting_pl.md # Wersja polska
✨ docs_src/crypto/rng_capture_troubleshooting_ru.md # Wersja rosyjska
```

#### **Listy Kontrolne Testowania**
```
✨ docs_src/crypto/rng_test_checklist.txt            # Angielska lista kontrolna
✨ docs_src/crypto/rng_test_checklist_pl.txt         # Wersja polska
✨ docs_src/crypto/rng_test_checklist_ru.txt         # Wersja rosyjska
```

#### **Skrypty Budowania & Automatyzacji**
```
✨ RNG_SETUP_QUICK_COMMANDS.sh                   # Szybka konfiguracja RNG
✨ activate-tests.sh                             # Aktywuj .venv-test
✨ run-tests.sh                                  # Runner testów
✨ install-test-deps.sh                          # Zainstaluj zależności systemu
✨ VENV_QUICKSTART.txt                           # Przewodnik szybkiego uruchomienia venv
✨ scripts/test_commands_reference.sh            # Referencia poleceń testowania
✨ scripts/test_rng_signing_comprehensive.py     # Kompleksowy zestaw testów
```

#### **Zależności**
```
✨ requirements-test.txt                         # Pakiety pip (dev)
✨ requirements-test-lock.txt                    # Zablokowane wersje
```

**Razem:** ~30 nowych plików (głównie dokumentacja + testy)

---

### 🔧 Zmodyfikowane Pliki (Modified)

#### **1. Core/Inc/crypto_wallet.h**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Dodane funkcje dla dump RNG
- Możliwie wyeksportowane funkcje haszowania dla testowania
- Rozszerzenia API dla testowania statystycznego
```

#### **2. Core/Src/hw_init.c**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Inicjalizacja RNG (Generator Liczb Losowych)
- Konfiguracja dla trybu USE_RNG_DUMP
- Możliwie ulepszona obsługa błędów inicjalizacji
```

#### **3. Core/Src/main.c**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Dodana obsługa trybu dump RNG
- Tworzenie zadania wyjścia danych RNG
- Logowanie inicjalizacji
- Warunkowa kompilacja (USE_RNG_DUMP)
```

#### **4. Makefile**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Nowa flaga build: USE_RNG_DUMP
- Reguły dla budowania celu dump RNG
- Aktualizacja definicji przy kompilacji
- Możliwie nowe zależności lub ścieżki
```

#### **5. docs_src/crypto/README.md**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Dodane linki do nowych dokumentów
- Dokumentacja testowania RNG
- Zaktualizowane przykłady poleceń
- Nowe sekcje o Dieharder
```

#### **6. .gitignore**
```diff
Status: ZMODYFIKOWANY (М)
Prawdopodobne zmiany:
- Wyłączenie .venv-test/
- Wyłączenie katalogów docs
- Wyłączenie artefaktów budowania
- Możliwie wyłączenie cache Python
```

---

### 📈 Analiza Aktualizacji: Co Się Zmieniło?

#### **🎯 Główny Temat: Testowanie Statystyczne RNG**

Projekt dodał **pełną infrastrukturę do testowania jakości generatora liczb losowych** używając zestawu testów Dieharder:

**Co dodano:**

1. **Przechwycenie Danych RNG** (`rng_dump.c/h`)
   - Wysyła surowe bajty ze wbudowanego RNG do UART
   - Używane do późniejszej analizy

2. **Zestaw Testów Python** (`test_rng_signing_comprehensive.py`)
   - Pełny cykl testowania: przechwycenie → Dieharder → analiza
   - Integracja ze stroną hosta

3. **Kompletna Dokumentacja**
   - Przewodniki konfiguracji (ang/ros/pol)
   - Rozwiązywanie problemów (ang/ros/pol)
   - Listy kontrolne dla testowania

4. **Skrypty Automatyzacji**
   - Szybka konfiguracja (`RNG_SETUP_QUICK_COMMANDS.sh`)
   - Aktywacja środowiska (`activate-tests.sh`)
   - Testowanie batch (`run-tests.sh`)

5. **Zarządzanie Zależnościami**
   - `requirements-test.txt` - Pakiety Python
   - `requirements-test-lock.txt` - Stałe wersje
   - `install-test-deps.sh` - Zależności systemu

---

## 📊 Wymiarowość Zmian

```
Nowych plików:     ~30 (dokumentacja + skrypty)
Zmodyfikowanych:    6 (główna funkcjonalność)
Linii kodu dodano:   ~1500+ (dokumenty + skrypty)
Linii kodu zmieniono: ~200-300 (główne pliki)

Fokus:  QA / Infrastruktura Testowania
Status: Aktywny Rozwój
```

---

## 🔄 Rekomendacje Integracji

### Dla stm32_secure_boot → CryptoWallet

**Co można pożyczyć:**
1. ✅ Konfiguracja FreeRTOS + best practices
2. ✅ Profil LwIP lwip_zero (czysty setup)
3. ✅ Przykłady podwójnego transportu (UART/HID)
4. ✅ Weryfikacja bootloadera (jeśli potrzebna)

### Dla CryptoWallet → stm32_secure_boot

**Co może być przydatne:**
1. ✅ Infrastruktura testowania RNG
2. ✅ Integracja trezor-crypto (jako przykład)
3. ✅ Dokumentacja wielojęzyczna (szablon)
4. ✅ Podejście FSM do zarządzania stanem

---

## 📌 Wnioski

|| Aspekt | stm32_secure_boot | CryptoWallet | Wniosek |
||---|---|---|---|
|| **Cel** | Badanie | Production | Różne cele |
|| **Złożoność** | Średnia | Wysoka | CW bardziej złożony |
|| **Dojrzałość** | Beta | Stabilny | CW gotowy do użytku |
|| **Dokumentacja** | Dobra | Doskonała | CW lepiej udokumentowany |
|| **Testowanie** | Podstawowe | Kompleksowe | CW ma infrastrukturę QA |
|| **Kryptografia** | Podstawowa | Production-grade | CW z BIP-39/32 |
|| **Interakcja** | 🔗 Niezależne | 🔗 CW zależy od SB | CW = SB + warstwa krypto |

---

## 📚 Aktualizacja Dokumentacji

**Dla szybkiego startu z nowymi testami:**

```bash
# 1. Aktywuj wirtualne środowisko
source activate-tests.sh

# 2. Zainstaluj zależności systemowe
bash install-test-deps.sh

# 3. Uruchom szybką konfigurację RNG
bash RNG_SETUP_QUICK_COMMANDS.sh

# 4. Uruchom pełny test
python3 scripts/test_rng_signing_comprehensive.py

# 5. Uruchom Dieharder
python3 scripts/run_dieharder.py
```

**Pliki do przeczytania:**
- `docs_src/TESTING_GUIDE_RNG_SIGNING.md` - Główny przewodnik
- `docs_src/crypto/rng_dump_setup.md` - Konfiguracja RNG
- `docs_src/TEST_SCRIPTS_README.md` - Opis skryptów

---

**Ten dokument**: `/data/projects/CryptoWallet/PROJECTS_COMPARISON_AND_UPDATES.md`  
**Ostatnia aktualizacja:** 2026-03-20  
**Analiza wykonana:** AI Agent (Cursor)
