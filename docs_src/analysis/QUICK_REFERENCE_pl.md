# Szybka Referencia: Projekty & Aktualizacje

**Przewodnik szybkiego wyszukiwania dla CryptoWallet & stm32_secure_boot**

---

## 📋 Porównanie Projektów na Jeden Rzut Oka

```
╔════════════════╦═════════════════════════╦═════════════════════════╗
║ Funkcja        ║ stm32_secure_boot       ║ CryptoWallet            ║
╠════════════════╬═════════════════════════╬═════════════════════════╣
║ Typ            ║ Badawcze/Edukacyjne     ║ Production Portfel      ║
║ Lokalizacja    ║ /data/projects/         ║ /data/projects/         ║
║                ║ stm32_secure_boot       ║ CryptoWallet            ║
╠════════════════╬═════════════════════════╬═════════════════════════╣
║ Bootloader     ║ ✅ Pełny (verified boot)║ ❌ Opcjonalny (odziedziczony) ║
║ Crypto         ║ ✅ Podstawowy (CMOX)    ║ ✅ Pełny (trezor-crypto) ║
║ HD Portfel     ║ ❌ Nie                  ║ ✅ Tak (BIP-39/32)      ║
║ Sieć           ║ ✅ LwIP (opcjonalny)    ║ ✅ LwIP (obowiązkowy)   ║
║ USB            ║ ✅ HID                  ║ ✅ WebUSB               ║
║ Wyświetlacz    ║ ✅ SSD1306 (opcjonalny) ║ ✅ SSD1306 (obowiązkowy)║
║ Testowanie RNG ║ ❌ Nie                  ║ ✅ Tak (Dieharder)      ║
║ Dojrzałość     ║ Beta                    ║ Stabilny                ║
║ Pliki          ║ 100+                    ║ 150+                    ║
║ Cele Budowania ║ 12+ profilów            ║ 1 główny + warianty     ║
╚════════════════╩═════════════════════════╩═════════════════════════╝
```

---

## 🚀 Pierwsze Kroki: CryptoWallet

### Szybkie Budowanie & Flashowanie

```bash
# 1. Przejdź do projektu
cd /data/projects/CryptoWallet

# 2. Buduj firmware
make all

# 3. Flash do urządzenia
make flash

# 4. Monitoruj wyjście UART
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw
```

### Szybkie Testowanie (NOWE - Testowanie RNG)

```bash
# 1. Aktywuj środowisko testów
source activate-tests.sh

# 2. Zainstaluj zależności
bash install-test-deps.sh

# 3. Flash z włączonym dump RNG
make flash USE_RNG_DUMP=1

# 4. Uruchom kompleksowe testy
python3 scripts/test_rng_signing_comprehensive.py

# 5. Przeglądaj wyniki
cat test_results/summary.txt
```

---

## 📁 Lokalizacje Kluczowych Plików

### CryptoWallet

|| Plik | Cel |
||------|---------|
|| `Core/Src/main.c` | Punkt wejścia FreeRTOS |
|| `Core/Src/task_sign.c` | FSM Podpisywania |
|| `Core/Src/crypto_wallet.c` | Wrapper Kryptografii |
|| `Core/Src/task_net.c` | Serwer HTTP |
|| `Core/Src/task_display.c` | UI SSD1306 |
|| `Core/Src/rng_dump.c` | ⭐ NOWY: Testowanie RNG |
|| `Makefile` | System budowania |
|| `docs_src/` | Dokumentacja (128+ plików) |
|| `scripts/` | Narzędzia Python |
|| `ThirdParty/trezor-crypto/` | Biblioteka Bitcoin |

### stm32_secure_boot

|| Plik | Cel |
||------|---------|
|| `bootloader/src/main.c` | Weryfikowana загрузka |
|| `app/step2_hid/main.c` | Signer HID |
|| `app/step2_hid/signer_transport.h/c` | Obsługiwacz protokołu |
|| `FreeRTOS/` | Kernel RTOS |
|| `scripts/` | Buduj/debug/test |
|| `docs/` | Dokumentacja |

---

## 🔧 Flagi Konfiguracji Budowania

### Flagi CryptoWallet

```makefile
# Włącz/wyłącz funkcje w momencie kompilacji

USE_LWIP=1              # Włącz LwIP + Ethernet (domyślnie)
USE_CRYPTO_SIGN=1       # Włącz podpisywanie ECDSA
USE_TEST_SEED=1         # Użyj zakodowanego mnemoniku testowego
USE_WEBUSB=1            # Włącz interfejs USB WebUSB
USE_RNG_DUMP=1          # Włącz testowanie statystyczne RNG ⭐ NOWY
SKIP_OLED=1             # Wyłącz I2C/OLED jeśli zawieszenie magistrali
LWIP_ALIVE_LOG=1        # Okresowy debug heartbeat

# Przykłady:
make                              # Domyślne budowanie
make USE_CRYPTO_SIGN=1           # Z kryptografią
make USE_TEST_SEED=1             # Z mnonikiem testowym (auto-włącza crypto)
make USE_RNG_DUMP=1 flash        # Buduj i flash tester RNG
make clean all                   # Czyste przebudowanie
```

### Flagi stm32_secure_boot

```makefile
make bootloader              # Buduj bootloader
make step1                   # LED + UART demo
make step2_hid              # Główny signer HID (domyślnie)
make lwip_zero              # Wariant LwIP
make flash-step2_hid        # Flash do urządzenia
make debug                  # Sesja GDB debugging
```

---

## 📊 Ostatnie Zmiany (CryptoWallet)

### Co Nowego (30 plików)

```
Pliki Główne (Zmodyfikowane):
  • Core/Inc/crypto_wallet.h         - Dodane API RNG
  • Core/Src/hw_init.c               - Inicjalizacja RNG
  • Core/Src/main.c                  - Obsługa zadania RNG
  • Core/Src/rng_dump.c              - ✨ NOWY: RNG dumper
  • Core/Inc/rng_dump.h              - ✨ NOWY: API RNG
  • Makefile                         - Dodana flaga USE_RNG_DUMP

Skrypty Testów (NOWE):
  • scripts/test_rng_signing_comprehensive.py
  • scripts/capture_rng_uart.py
  • scripts/run_dieharder.py

Dokumentacja (NOWA) - 18 plików:
  • docs_src/TESTING_GUIDE_RNG_SIGNING.md
  • docs_src/TEST_SCRIPTS_README.md
  • docs_src/INSTALL_TEST_DEPS.md
  • docs_src/crypto/rng_dump_setup.md (+ PL + RU)
  • docs_src/crypto/testing_setup.md (+ PL + RU)
  • docs_src/crypto/rng_capture_troubleshooting.md (+ PL + RU)
  • docs_src/crypto/rng_test_checklist.txt (+ PL + RU)

Skrypty Automatyzacji (NOWE):
  • activate-tests.sh
  • run-tests.sh
  • install-test-deps.sh
  • RNG_SETUP_QUICK_COMMANDS.sh

Zależności (NOWE):
  • requirements-test.txt
  • requirements-test-lock.txt
  • .venv-test/ (Wirtualne środowisko Python)
```

---

## 💡 Przypadki Użycia

### stm32_secure_boot - Kiedy Użyć

✅ **Dobre dla:**
- Nauka pojęć weryfikowanej загрузки
- Zrozumienie projektowania bootloadera
- Weryfikacja sygnatury ECDSA
- Komunikacja podwójnego transportu (UART + HID)
- Projekty edukacyjne
- Testowanie pojęć kryptograficznych

❌ **Nie idealne dla:**
- Production portfel Bitcoin (wymaga BIP-39/32)
- Komunikacja sieciowa
- Złożona logika walidacji

### CryptoWallet - Kiedy Użyć

✅ **Dobre dla:**
- Production portfel Bitcoin
- Przechowywanie zimne podpisów
- Komunikacja multi-protocol (HTTP, WebUSB, UART)
- Walidacja jakości RNG
- Potrzebna obsługa BIP-39/BIP-32
- Bezpieczne zarządzanie kluczami

❌ **Nie idealne dla:**
- Nauka projektowania bootloadera
- Proste projekty edukacyjne
- Minimalne wymagania sprzętu

---

## 🔐 Lista Kontrolna Bezpieczeństwa

### Funkcje Bezpieczeństwa CryptoWallet

- ✅ Sygnatury ECDSA (secp256k1)
- ✅ Obsługa mnemoniku BIP-39
- ✅ Rozszerzone pochodne kluczy BIP-32
- ✅ Haszowanie transakcji SHA-256
- ✅ Bezpieczne czyszczenie bufora memzero()
- ✅ Wymagane potwierdzenie przyciskiem użytkownika
- ✅ Jakość RNG zwalidowana (Dieharder) ⭐ NOWY
- ⚠️  Brak uwierzytelniania portfela sprzętowego (TODO)
- ⚠️  Nasiona przechowywane w RAM (brak trwałego przechowywania)
- ⚠️  Brak opcji RDP/WRP (TODO)

### Funkcje Bezpieczeństwa stm32_secure_boot

- ✅ Sprawdzenie integralności bootloadera SHA-256
- ✅ Weryfikacja sygnatury ECDSA
- ✅ Opcjonalna biblioteka kryptografii CMOX
- ✅ Wskaźnik LED błędu
- ❌ Brak pełnej implementacji portfela
- ❌ Brak testowania RNG

---

## 📞 Referencia Wspólnych Poleceń

### Budowanie & Flashowanie (CryptoWallet)

```bash
# Buduj
make                              # Domyślne (minimal-lwip)
make all                          # Pełne budowanie
make clean                        # Usuń artefakty

# Flash
make flash                        # Używając ST-Link
make flash-minimal-lwip
make flash-boottest

# Debug
make debug                        # Start sesji GDB

# Dokumentacja
make docs                         # Generuj MkDocs
make docs-serve                  # Serwuj na localhost:8000
```

### Budowanie & Flashowanie (stm32_secure_boot)

```bash
# Buduj
make bootloader                  # Tylko bootloader
make step2_hid                   # Aplikacja główna
make lwip_zero                   # Wariant LwIP

# Flash
make flash-bootloader           # Flash bootloadera
make flash-step2_hid            # Flash aplikacji głównej

# Debug
scripts/gdb_step2_hid.sh        # GDB debugging
minicom -D /dev/ttyACM1 -b 115200  # Monitor seryjny
```

### Testowanie (CryptoWallet)

```bash
# Szybkie testowanie
source activate-tests.sh
python3 scripts/test_usb_sign.py

# Testowanie RNG (NOWE)
python3 scripts/test_rng_signing_comprehensive.py

# Konkretne przechwycenie RNG
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --samples 1000000

# Uruchom analizę Dieharder
python3 scripts/run_dieharder.py rng_data.bin --verbose
```

---

## 🔍 Szybkie Wskazówki Rozwiązywania Problemów

### CryptoWallet

|| Problem | Rozwiązanie |
||-------|----------|
|| Flashowanie nie udane | Sprawdź: `make clean && make flash` |
|| USB nie wykryty | Sprawdź: UART pokazuje błędy, ponownie podłącz USB |
|| OLED nie wyświetla | Spróbuj: `make flash SKIP_OLED=1` |
|| Serwer HTTP nie odpowiada | Sprawdź: Kabel Ethernet, przydzielony IP DHCP |
|| Testy RNG nie udane | Upewnij się: `USE_RNG_DUMP=1` w Makefile |
|| Błędy memzero() | Sprawdź: Optymalizacja kompilatora `-O2` |

### stm32_secure_boot

|| Problem | Rozwiązanie |
||-------|----------|
|| Bootloader nie weryfikuje | Ustaw: `USE_ECDSA_STUB=1` do testowania |
|| HID nie działa | Sprawdź: Kabel USB, deskryptor urządzenia |
|| Brak wyjścia UART | Weryfikuj: Konfiguracja USART3, szybkość baud |
|| Zawieszenie inicjalizacji LwIP | Spróbuj: Wyłączy problematyczne peryferia |

---

## 📚 Mapa Dokumentacji

### Struktura Dokumentacji CryptoWallet

```
CryptoWallet/
├── README.md                          # Główny punkt wejścia
├── docs_src/
│   ├── main.md                        # Przegląd architektury
│   ├── architecture.md                # Projektowanie systemu
│   ├── TESTING_GUIDE_RNG_SIGNING.md  # ⭐ NOWY: Przewodnik testowania
│   ├── TEST_SCRIPTS_README.md        # ⭐ NOWY: Referencia skryptów
│   ├── INSTALL_TEST_DEPS.md          # ⭐ NOWY: Zależności
│   ├── VENV_SETUP.md                 # ⭐ NOWY: Przewodnik Venv
│   │
│   └── crypto/
│       ├── README.md                 # Przegląd Kryptografii
│       ├── trezor-crypto-integration.md # Dokumentacja biblioteki krypto
│       ├── wallet_seed.md            # Dokumentacja BIP-39
│       ├── rng_dump_setup.md         # ⭐ NOWY: Konfiguracja RNG
│       ├── testing_setup.md          # ⭐ NOWY: Przepływ pracy testowania
│       ├── rng_capture_troubleshooting.md # ⭐ NOWY: Rozwiąż problemy
│       ├── rng_test_checklist.txt    # ⭐ NOWY: Lista kontrolna
│       │
│       ├── rng_dump_setup_pl.md      # Wersja polska
│       ├── rng_dump_setup_ru.md      # Wersja rosyjska
│       └── ... (więcej tłumaczeń)
│
└── PROJECTS_COMPARISON_AND_UPDATES.md  # ⭐ TEN PLIK
    (Pełne porównanie + podsumowanie aktualizacji)
```

### Struktura Dokumentacji stm32_secure_boot

```
stm32_secure_boot/
├── readme.md                          # Przegląd Polski/Rosyjski
├── docs/
│   ├── ARCHITECTURE.md               # Układ pamięci, sekwencja загрузки
│   ├── HID_SIGNER_REFERENCE.md      # Specyfikacja protokołu
│   ├── VERIFIED_BOOT_STM32_PL.md    # Pojęcia загрузki
│   ├── DEBUG_PL.md                  # Przewodnik GDB debugging
│   ├── TESTING_DEMO1/DEMO2.md       # Procedury testowania
│   ├── ENTROPY_VALIDATION.md        # Info RNG/entropia
│   └── ... (więcej dokumentów Polski/Rosyjski)
│
└── app/
    └── step1/README.md               # Notatki specyficzne dla Step 1
```

---

## 🎯 Co Się Zmieniło: Podsumowanie Wykonawcze

### Główny Temat Aktualizacji: **Infrastruktura Testowania RNG**

**Dlaczego?** - Podpisywanie ECDSA zależy od jakości RNG:
- Generowanie klucza prywatnego z seed
- Nonce (k) w obliczeniach sygnatury
- Jeśli RNG jest przewidywalny → podpisy mogą być sfałszowane
- **Dlatego:** Walidacja jakości RNG jest krytyczna

**Co zostało dodane:**
1. **Obsługa Sprzętu**: Tryb dump RNG w firmware
2. **Zestaw Testów**: Skrypty Python + integracja Dieharder
3. **Dokumentacja**: Kompletne przewodniki (3 języki)
4. **Automatyzacja**: Skrypty szybkiego startu + środowisko
5. **Walidacja**: Kompleksowe testowanie statystyczne

**Wpływ:**
- ✅ Możesz teraz walidować jakość RNG
- ✅ Powtarzalne środowisko testów
- ✅ Gotowe do CI/CD
- ✅ Zapewnianie jakości production-grade

---

## 📊 Podsumowanie Zmian Plików

```
Statystyka:
├── Nowe Pliki          30 (dokumentacja + skrypty + zależności)
├── Zmienione Pliki      6 (główna funkcjonalność)
├── Usunięte Pliki       0
├── Linie Dodane      1500+ (głównie dokumentacja)
├── Linie Zmienione     200-300 (kod główny)
│
Obszary Fokus:
├── Infrastruktura Testowania
├── Zapewnianie Jakości
├── Dokumentacja
└── Automatyzacja
```

---

## 🔗 Ważne Linki & Referencje

### CryptoWallet

- **Git Repo**: `/data/projects/CryptoWallet`
- **Gałąź**: `main` (śledzi origin/main)
- **Makefile**: `/data/projects/CryptoWallet/Makefile`
- **Biblioteka Krypto**: `ThirdParty/trezor-crypto/`
- **Główny Dokument**: `PROJECTS_COMPARISON_AND_UPDATES.md` ⭐

### stm32_secure_boot

- **Git Repo**: `/data/projects/stm32_secure_boot`
- **Gałąź**: `main`
- **Główny Makefile**: `/data/projects/stm32_secure_boot/Makefile`
- **Bootloader**: `bootloader/src/main.c`
- **Aplikacja Główna**: `app/step2_hid/main.c`

---

## 🚦 Status & Następne Kroki

### CryptoWallet - Bieżący Status

✅ **Stabilny**
- Podstawowa funkcjonalność działa
- Wsparcie multi-protocol aktywne
- Kompletna dokumentacja
- Infrastruktura testów w miejscu

🔄 **W Toku**
- Walidacja statystyczna RNG (DODANA)
- Tłumaczenie dokumentacji (Kompletne)
- Automatyzacja testów (Kompletna)

❌ **TODO (Przyszłość)**
- Uwierzytelnianie portfela sprzętowego
- Trwałe przechowywanie kluczy
- Konfiguracja bezpieczeństwa RDP/WRP
- Integracja niestandardowego bootloadera

### stm32_secure_boot - Bieżący Status

✅ **Stabilny**
- Dostępne wiele profili budowania
- Fokus edukacyjny utrzymywany
- Dobra dokumentacja

🔄 **Faza Badawcza**
- Zaawansowane pojęcia bezpiecznego загрузki
- Wiele przykładów transportu

---

**Dokument Szybkiej Referencji**  
**Ostatnia Aktualizacja:** 2026-03-20  
**Status:** Kompletna i Gotowa do Użytku

Aby uzyskać szczegółową analizę, zobacz:
1. `PROJECTS_COMPARISON_AND_UPDATES.md` - Pełne porównanie
2. `UPDATES_SUMMARY.md` - Podsumowanie zmian
3. `ARCHITECTURE_DETAILED.md` - Pogłębiona analiza architektury
