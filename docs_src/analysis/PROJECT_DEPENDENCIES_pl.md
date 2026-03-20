# Zależności Projektu & Relacje

**Zrozumienie sposobu, w jaki stm32_secure_boot i CryptoWallet się łączą**

---

## 🔗 Graf Zależności

```
Zależności Zewnętrzne (Strony trzecie):
├─ STM32CubeH7/                    (HAL ST Microelectronics)
│  └─ Używane przez: [oba projekty]
│
├─ STM32CubeExpansion_Crypto/      (Biblioteka kryptografii CMOX)
│  └─ Używane przez: stm32_secure_boot (bootloader)
│
├─ Kernel FreeRTOS                 (RTOS)
│  ├─ Wersja w stm32_secure_boot/FreeRTOS/
│  └─ Używane przez: [oba projekty przez stm32_secure_boot]
│
├─ Stos sieciowy LwIP              (Stos TCP/IP)
│  ├─ Wersja w stm32_secure_boot/ (profil lwip_zero)
│  └─ Używane przez: CryptoWallet (obowiązkowy)
│
└─ trezor-crypto/                  (Biblioteka Bitcoin)
   ├─ Lokalizacja: CryptoWallet/ThirdParty/
   └─ Używane przez: TYLKO CryptoWallet

Zależności Wewnętrzne:
stm32_secure_boot/
├─ Niezależny
├─ Można budować oddzielnie
├─ Dostarcza bootloader + przykłady aplikacji
└─ Używane jako referenca przez CryptoWallet

            ↓ (UŻYWANE JAKO FUNDAMENT)

CryptoWallet/
├─ Zależy od FreeRTOS z stm32_secure_boot
├─ Zależy od LwIP (lwip_zero)
├─ Zależy od sterowników HAL
└─ Dodaje: trezor-crypto, portfele HD, pełny stos podpisywania
```

---

## 📦 Drzewo Zależności Komponentów

### stm32_secure_boot (Niezależny)

```
┌─ stm32_secure_boot (SAMODZIELNY)
│
├─ bootloader/ ──┬─ Rdzeń Bootloadera
│                ├─ SHA-256 (wbudowany)
│                ├─ ECDSA (CMOX lub stub)
│                └─ Przechowywanie kluczy
│
└─ app/ ─────────┬─ step2_hid/ (GŁÓWNY)
                 │  ├─ FreeRTOS
                 │  ├─ UART + USB HID
                 │  ├─ Przycisk + OLED
                 │  └─ Transport signer
                 │
                 ├─ lwip_zero/
                 │  ├─ FreeRTOS
                 │  ├─ LwIP
                 │  └─ Serwer HTTP
                 │
                 └─ step1, step2, itp.
```

### CryptoWallet (Złożony)

```
┌─ CryptoWallet (POCHODNE)
│
├─ Zależności:
│  ├─ FreeRTOS (z stm32_secure_boot)
│  ├─ LwIP (z stm32_secure_boot/lwip_zero)
│  ├─ STM32CubeH7 (HAL)
│  └─ trezor-crypto (ThirdParty/)
│
├─ Główne Funkcje:
│  ├─ Architektura oparta na zadaniach (FreeRTOS)
│  ├─ Multi-protocol (LwIP + WebUSB + UART)
│  ├─ Portfel HD (BIP-39/32)
│  ├─ Walidacja TX
│  ├─ FSM Podpisywania
│  └─ Testowanie RNG ✨
│
└─ Nie zawiera bootloadera
   (Może dziedziczyć z stm32_secure_boot jeśli życzysz)
```

---

## 📊 Macierz Współużytkowania & Ponownego Wykorzystania Kodu

|| Komponent | stm32_secure_boot | CryptoWallet | Notatki |
||---|---|---|---|
|| **FreeRTOS** | ✅ Dołączony | ✅ Używa wersji SB | Wspólny kernel |
|| **LwIP** | ✅ lwip_zero | ✅ Używa wersji SB | Stos sieciowy |
|| **HAL STM32** | ✅ Zewnętrzny | ✅ Zewnętrzny | Oba używają tego samego HAL |
|| **Sterowniki UART/USB/I2C** | ✅ Dołączony | ✅ Używa/rozszerza SB | Częściowo wspólne |
|| **Bootloader** | ✅ Pełny | ❌ Opcjonalny | Przewaga SB |
|| **Krypto ECDSA** | ✅ CMOX | ✅ trezor-crypto | Różne biblioteki |
|| **Portfel HD** | ❌ Nie | ✅ Tak | Specyficzne dla CW |
|| **Serwer HTTP** | ✅ lwip_zero | ✅ Ulepszone | CW poprawił |
|| **WebUSB** | ❌ Nie | ✅ Tak | Specyficzne dla CW |
|| **Testowanie RNG** | ❌ Nie | ✅ Tak | Specyficzne dla CW ✨ |

---

## 🔄 Integracja Systemu Budowania

### Jak Budować

#### stm32_secure_boot Samodzielnie

```bash
cd /data/projects/stm32_secure_boot

# Bootloader
make bootloader
arm-none-eabi-objcopy -O binary build/bootloader.elf build/bootloader.bin

# Aplikacja (step2_hid)
make step2_hid
arm-none-eabi-objcopy -O binary build/app/app.elf build/app/app.bin

# Flash
st-flash write build/bootloader.bin 0x08000000
st-flash write build/app/app.bin 0x08010000
```

#### CryptoWallet Z Opcjonalnym Bootloaderem

```bash
cd /data/projects/CryptoWallet

# Opcja 1: Buduj z bootloaderem stm32_secure_boot (rekomendowane)
cd /data/projects/stm32_secure_boot && make bootloader
cp build/bootloader.bin ../CryptoWallet/build/

# Opcja 2: Buduj tylko CryptoWallet (tylko aplikacja)
cd /data/projects/CryptoWallet
make
arm-none-eabi-objcopy -O binary build/cryptowallet.elf build/cryptowallet.bin

# Flash (CryptoWallet na adresie aplikacji)
st-flash write build/cryptowallet.bin 0x08010000

# Lub flash z bootloaderem
st-flash write build/bootloader.bin 0x08000000
```

---

## 🏗️ Układ Pamięci Z Oboma Projektami

### Jeśli Używasz Bootloadera stm32_secure_boot + Aplikacji CryptoWallet

```
Układ Pamięci Flash:
0x08000000 ┌──────────────────────────────────┐
           │  BOOTLOADER (z stm32_sb)         │  64 KB
           │  ├─ Kod bootloadera              │
           │  ├─ Weryfikacja SHA-256          │
           │  ├─ Weryfikacja ECDSA (CMOX)     │
           │  ├─ Klucze publiczne             │
           │  └─ Raportowanie błędów LED     │
0x08010000 ├──────────────────────────────────┤
           │  APLIKACJA (CryptoWallet)        │  1.5 MB
           │  ├─ Kernel FreeRTOS              │
           │  ├─ Stos LwIP                    │
           │  ├─ trezor-crypto (Bitcoin)      │
           │  ├─ Serwer HTTP                  │
           │  ├─ Interfejs WebUSB             │
           │  ├─ Menedżery zadań              │
           │  │  ├─ task_sign.c               │
           │  │  ├─ task_net.c                │
           │  │  ├─ task_display.c            │
           │  │  ├─ task_user.c               │
           │  │  └─ task_io.c                 │
           │  ├─ Testowanie RNG (NOWE)        │
           │  └─ Sterownik OLED (SSD1306)     │
0x08180000 ├──────────────────────────────────┤
           │  [Wolna przestrzeń - niewykorzystana]│  512 KB
           │                                  │
0x08200000 └──────────────────────────────────┘

Układ RAM:
0x20000000 ┌──────────────────────────────────┐
           │  Kernel FreeRTOS                 │
           │  ├─ Bloki kontroli zadań (TCB)   │  100 KB
           │  ├─ Listy gotowości              │
           │  └─ Struktury kolejek/zdarzeń   │
0x20019000 ├──────────────────────────────────┤
           │  Obiekty IPC (Kolejki, Mutexy)   │
           │  ├─ Kolejka tx_request           │  50 KB
           │  ├─ Kolejka sign_response        │
           │  ├─ Kolejka wyświetlacza         │
           │  ├─ Kolejka dziennika UART       │
           │  └─ Mutexy/semafory             │
0x20026000 ├──────────────────────────────────┤
           │  Stosy Zadań (5 zadań)           │  200 KB
           │  ├─ Stos zadania podpisywania    │  (32 KB każdy)
           │  ├─ Stos zadania sieci           │
           │  ├─ Stos zadania wyświetlacza    │
           │  ├─ Stos zadania użytkownika     │
           │  └─ Stos zadania IO              │
0x20046000 ├──────────────────────────────────┤
           │  Sterta (dynamiczna alokacja)    │  250 KB
           │  ├─ Bufory LwIP                  │
           │  ├─ malloc/free (biblioteka krypto) │
           │  └─ Bufory USB                   │
0x200A2000 ├──────────────────────────────────┤
           │  Deskryptory LwIP RX + dane      │  96 KB
           │  ├─ Bufor ramki Ethernet         │
           │  ├─ Bufory UDP/TCP               │
           │  └─ Łańcuchy mbuf                │
0x200C0000 ├──────────────────────────────────┤
           │  [Pozostała przestrzeń]          │  192 KB
           │                                  │
0x20100000 └──────────────────────────────────┘
```

---

## 🔐 Łańcuch Zaufania Bezpieczeństwa

### Z Bootloaderem stm32_secure_boot

```
┌─────────────────────────────────────────┐
│  STM32H743 System Flash @ 0x08000000    │
│                                          │
│  ┌──────────────────────────────────┐  │
│  │  Bootloader ROM Cortex-M7        │  │
│  │  (1 KB, niezmienny z fabryki)    │  │
│  │                                   │  │
│  │  Odpowiedzialność:               │  │
│  │  └─ Załaduj & wykonaj            │  │
│  │     bootloader użytkownika       │  │
│  │     @ 0x08000000                 │  │
│  └──────────────────────────────────┘  │
│            ↓ Skok do ↓                  │
│  ┌──────────────────────────────────┐  │
│  │  Bootloader Użytkownika (64 KB)  │  │
│  │  (z stm32_secure_boot)           │  │
│  │                                   │  │
│  │  Odpowiedzialność:               │  │
│  │  ├─ Oblicz SHA-256 aplikacji    │  │
│  │  ├─ Weryfikuj sygnaturę ECDSA   │  │
│  │  ├─ Skok do aplikacji jeśli OK   │  │
│  │  └─ Zatrzymaj + LED błędu jeśli  │  │
│  │     źle                          │  │
│  └──────────────────────────────────┘  │
│            ↓ Weryfikowany Skok ↓        │
│  ┌──────────────────────────────────┐  │
│  │  Aplikacja (1.5 MB)              │  │
│  │  (z CryptoWallet)                │  │
│  │                                   │  │
│  │  Odpowiedzialność:               │  │
│  │  ├─ Inicjalizuj sprzęt           │  │
│  │  ├─ Utwórz zadania FreeRTOS      │  │
│  │  ├─ Czekaj na wejście użytkownika│  │
│  │  ├─ Podpisz transakcje           │  │
│  │  └─ Zwróć podpisy                │  │
│  └──────────────────────────────────┘  │
└─────────────────────────────────────────┘

Łańcuch Zaufania:
Bootloader ROM
       ↓ ufa
Bootloader Użytkownika (podpisany ECDSA)
       ↓ ufa
Aplikacja (podpisana ECDSA)
       ↓ wykonuje
Firmware CryptoWallet
```

---

## 🎯 Scenariusze Integracji

### Scenariusz 1: Samodzielny stm32_secure_boot

```
Przypadek Użycia: Testowanie bootloadera + signer HID
├─ Buduj: make step2_hid
├─ Wdróż: Jeden plik binarny z bootloaderem + aplikacją
├─ Wynik: Aplikacja zweryfikowana bootloaderem
└─ Zaleta: Łańcuch weryfikowanego загрузki, ale bez funkcji portfela
```

### Scenariusz 2: Tylko CryptoWallet (Brak Bootloadera)

```
Przypadek Użycia: Portfel bez weryfikacji bootloadera
├─ Buduj: make
├─ Wdróż: Plik binarny CryptoWallet @ 0x08010000
├─ Flash: Bezpośrednio do 0x08010000 (pomiń 0x08000000)
└─ Zaleta: Więcej miejsca dla aplikacji, prostsze
   Wada: Brak weryfikacji загрузki
```

### Scenariusz 3: Bootloader stm32_secure_boot + CryptoWallet (REKOMENDOWANE)

```
Przypadek Użycia: Production portfel z weryfikacją загрузki
├─ Buduj:
│  1. cd stm32_secure_boot && make bootloader
│  2. cd CryptoWallet && make
├─ Wdróż:
│  1. Flash bootloader.bin @ 0x08000000
│  2. Podpisz aplikację CryptoWallet (opcjonalnie)
│  3. Flash cryptowallet.bin @ 0x08010000
├─ Wynik: Weryfikowany łańcuch загрузki + pełne funkcje portfela
└─ Zaleta: Maksimum bezpieczeństwa + funkcjonalności
   Proces: Bardziej złożony, wymaga podpisywania
```

### Scenariusz 4: Przepływ Pracy Rozwojowej

```
Faza 1: Tworzenie Bootloadera
├─ cd stm32_secure_boot
├─ make bootloader
├─ make flash-bootloader
└─ Test z step2_hid

Faza 2: Tworzenie Aplikacji
├─ cd CryptoWallet
├─ make
├─ make flash
└─ Test podpisywania/sieci

Faza 3: Testowanie Integracji
├─ Oba bootloader + aplikacja
├─ Pełna weryfikacja łańcucha
├─ Hartowanie bezpieczeństwa
└─ Wydanie Production
```

---

## 📈 Porównanie Metryk Kodu

### stm32_secure_boot

```
├─ Razem Plików:           ~150 plików
├─ Razem Linii Kodu:       ~50,000 LOC
│  ├─ Bootloader:         ~3,000 LOC
│  ├─ Aplikacja (step2_hid): ~8,000 LOC
│  ├─ FreeRTOS:           ~12,000 LOC
│  ├─ LwIP:               ~20,000 LOC
│  └─ Common/Przykłady:   ~7,000 LOC
│
├─ Profile Budowania:      12+
├─ Dokumentacja:           20+ plików
└─ Status:                 Badawcze/Edukacyjne
```

### CryptoWallet

```
├─ Razem Plików:           ~200 plików (łącznie dokumenty)
├─ Razem Linii Kodu:       ~60,000+ LOC
│  ├─ Rdzeń:               ~8,000 LOC
│  ├─ Zadania:             ~5,000 LOC
│  ├─ Krypto/Portfel:      ~3,000 LOC
│  ├─ Sieć/USB:            ~4,000 LOC
│  ├─ FreeRTOS:            ~12,000 LOC
│  ├─ LwIP:                ~20,000 LOC
│  └─ Dokumentacja:        ~8,000 LOC
│
├─ Warianty Budowania:     3 (główny, minimalny, test)
├─ Skrypty Python:         11+ skryptów testów/narzędzi
├─ Dokumentacja:           128+ plików markdown (EN/RU/PL)
├─ Infrastruktura Testów:  Kompletna (NOWA) ✨
└─ Status:                 Production-Ready
```

---

## 🚀 Jak Zintegrować stm32_secure_boot w CryptoWallet

### Metoda 1: Użyj Istniejących FreeRTOS/LwIP (Bieżący)

CryptoWallet już używa FreeRTOS i LwIP z stm32_secure_boot:

```bash
# Niejawna zależność
cd /data/projects/CryptoWallet

# FreeRTOS i LwIP to referencje do wersji stm32_secure_boot
# (jeśli nie dołączone bezpośrednio w CryptoWallet)
```

### Metoda 2: Adopt Bootloader stm32_secure_boot

```bash
# Opcja A: Skopiuj bootloader do repozytorium CryptoWallet
cp -r /data/projects/stm32_secure_boot/bootloader \
      /data/projects/CryptoWallet/

# Opcja B: Referencyj jako submoduł
cd /data/projects/CryptoWallet
git submodule add /data/projects/stm32_secure_boot bootloader

# Opcja C: Zmodyfikuj Makefile CryptoWallet
# Dodaj cel budowania bootloadera
cat >> Makefile << 'EOF'
bootloader:
    $(MAKE) -C ../stm32_secure_boot bootloader
    cp ../stm32_secure_boot/build/bootloader.bin build/
EOF
```

### Metoda 3: Użyj Testowania RNG z CryptoWallet w stm32_secure_boot

```bash
# Skopiuj infrastrukturę testowania RNG
cp /data/projects/CryptoWallet/scripts/test_rng_*.py \
   /data/projects/stm32_secure_boot/scripts/

# Skopiuj dokumentację
cp -r /data/projects/CryptoWallet/docs_src/crypto/rng* \
      /data/projects/stm32_secure_boot/docs/

# Teraz stm32_secure_boot może też testować RNG
cd /data/projects/stm32_secure_boot
make USE_RNG_DUMP=1
python3 scripts/test_rng_signing_comprehensive.py
```

---

## 🔧 Rozwiązywanie Zależności Budowania

### Sprawdzanie Zależności Makefile

```makefile
# Jak CryptoWallet zapewnia dostępność FreeRTOS/LwIP

FREERTOS_PATH ?= ../stm32_secure_boot/FreeRTOS
LWIP_PATH ?= ../stm32_secure_boot

check-deps:
    @if [ ! -d "$(FREERTOS_PATH)" ]; then \
        echo "BŁĄD: FreeRTOS nie znaleziony na $(FREERTOS_PATH)"; \
        exit 1; \
    fi
    @if [ ! -d "$(LWIP_PATH)" ]; then \
        echo "BŁĄD: LwIP nie znaleziony na $(LWIP_PATH)"; \
        exit 1; \
    fi

all: check-deps compile link
```

---

## 📋 Lista Kontrolna Weryfikacji Zależności

```bash
# Weryfikuj, czy wszystkie zależności są dostępne

✅ STM32CubeH7
   └─ Lokalizacja: /data/projects/STM32CubeH7/
   └─ Sprawdzenie: [ -d STM32CubeH7/Drivers ] && echo "OK"

✅ FreeRTOS
   └─ Lokalizacja: /data/projects/stm32_secure_boot/FreeRTOS/
   └─ Sprawdzenie: [ -f FreeRTOS/Source/tasks.c ] && echo "OK"

✅ LwIP
   └─ Lokalizacja: /data/projects/stm32_secure_boot/
   └─ Sprawdzenie: [ -d lwip ] && echo "OK"

✅ trezor-crypto
   └─ Lokalizacja: /data/projects/CryptoWallet/ThirdParty/trezor-crypto/
   └─ Sprawdzenie: [ -f crypto.h ] && echo "OK"

✅ Łańcuch narzędzi
   └─ Sprawdzenie: arm-none-eabi-gcc --version
   └─ Powinien: >= 11.0

✅ Narzędzia Budowania
   └─ Sprawdzenie: which make && which st-flash
   └─ Wymagane: GNU make, narzędzia ST-Link
```

---

**Dokument:** Zależności Projektu & Relacje  
**Zaktualizowano:** 2026-03-20  
**Status:** Kompletne
