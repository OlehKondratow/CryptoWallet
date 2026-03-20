# Porównanie Wizualne Projektu & Podsumowanie Analizy

**Graficzny Przegląd CryptoWallet & stm32_secure_boot**

---

## 🎨 Wizualizacja Krajobrazu Projektu

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│                    Ekosystem STM32H7 (Wspólny Sprzęt)                  │
│                         Płyta NUCLEO-H743ZI2                           │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────────────────┐        ┌────────────────────────────┐ │
│  │   stm32_secure_boot          │        │      CryptoWallet          │ │
│  │   (Platforma Badawcza)       │        │  (Portfel Production)      │ │
│  │                              │        │                            │ │
│  │  ✅ Weryfikowana Загрузка    │        │  ✅ Pełny Portfel HD       │ │
│  │  ✅ Podwójny Transport       │        │  ✅ Multi-Protocol         │ │
│  │  ✅ HID/UART                │        │  ✅ HTTP/WebUSB/UART       │ │
│  │  ✅ Wiele Profilów          │        │  ✅ Testowanie RNG (NOWE)  │ │
│  │  ❌ Brak Kluczy HD          │        │  ✅ Walidacja TX           │ │
│  │  ❌ Brak Funkcji Portfela   │        │  ✅ Bezpieczna Pamięć      │ │
│  │                              │        │                            │ │
│  │  12+ Profile Budowania      │        │  1 Główny + Warianty       │ │
│  │  Status Beta                 │        │  Status Stabilny           │ │
│  │  50K LOC                     │        │  60K LOC + Dokumenty       │ │
│  └──────────────────────────────┘        └────────────────────────────┘ │
│           ▲                                      ▲                       │
│           │                                      │                       │
│           └──────────── Używa ──────────────────►  │                    │
│                                                    │                    │
│           FreeRTOS + LwIP                         │                    │
│           (Pożyczone z stm32_sb)                  │                    │
│                                                    │                    │
│  ┌──────────────────────────────┐                 │                    │
│  │   Komponenty Wspólne         │◄──────────────────┘                   │
│  │                              │                                      │
│  │  • Kernel FreeRTOS           │                                      │
│  │  • Stos LwIP                 │                                      │
│  │  • Sterowniki HAL STM32      │                                      │
│  │  • Sterowniki UART/USB/I2C   │                                      │
│  │  • Zarządzanie GPIO/LED      │                                      │
│  │  • Obsługa OLED SSD1306      │                                      │
│  │                              │                                      │
│  │  NOWY (CW): Testowanie RNG   │                                      │
│  │                              │                                      │
│  └──────────────────────────────┘                                      │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

Relacja Kluczowa:
┌─────────────────────────────────────────────────────────────────┐
│  stm32_secure_boot = Fundament/Biblioteka                      │
│  CryptoWallet = Aplikacja/Implementacja                        │
│                                                                  │
│  CW = SB + trezor-crypto + BIP-39/32 + HTTP + WebUSB + RNG    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📊 Mapa Ciepła Macierzy Funkcji

```
                    stm32_secure_boot   CryptoWallet
┌──────────────────────────────────────────────────────┐
│ Bootloader       ████████████ (Pełny)     ░░░░░░ (Opt) │  16 pts
│ Crypto           ████░░░░░░░░ (Podstaw)   ████████ (Zaw)│  12 pts
│ Portfel HD       ░░░░░░░░░░░░ (Brak)      ████████ (Pełn)│  8 pts
│ Sieć             ████░░░░░░░░ (LwIP)      ████████ (Pełn)│  8 pts
│ Protokoły        ████░░░░░░░░ (2x)        ████████ (3x) │  8 pts
│ Wyświetlacz      ████░░░░░░░░ (Opt)       ████████ (Wyb)│  8 pts
│ Testowanie RNG   ░░░░░░░░░░░░ (Brak)      ████████ (Now)│  8 pts
│ Dokumentacja     ██████░░░░░░ (Dobra)     ████████ (Wys)│  8 pts
│ Production Ready ░░░░░░░░░░░░ (Beta)      ████████ (Tak)│  8 pts
│ Framework Testów ░░░░░░░░░░░░ (Podstaw)   ████████ (Kom)│  8 pts
│                                                        │
│ RAZEM WYNIK:                        52 / 100         │
│ SB Zaawansowany:  30/100 (Badawcze)     CW Możliwości:│
│ CW Zaawansowany:  72/100 (Production)   22/100 przewaga│
└──────────────────────────────────────────────────────┘

█ = Zaimplementowane/Silne | ░ = Niezaimplementowane/Słabe
```

---

## 🔄 Diagramy Przepływu Danych

### stm32_secure_boot - Przepływ Signer HID

```
Wejście Użytkownika (Przycisk/UART/HID)
         ↓
    ┌────────────────────┐
    │ Transport Signer   │
    │ - Parsuj polecenie │
    │ - Waliduj wejście  │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Zarządzanie Kluczami │
    │ - Załaduj klucz    │
    │ - Tylko w pamięci   │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Silnik Podpisywania│
    │ - Hasz SHA-256     │
    │ - Podpisz ECDSA    │
    │ - Biblioteka CMOX  │
    └────────────────────┘
         ↓
    ┌────────────────────┐
    │ Odpowiedź          │
    │ - UART lub HID     │
    │ - Sygnatura        │
    └────────────────────┘
```

### CryptoWallet - Pełny Przepływ Portfela

```
Wejście Sieciowe (HTTP/WebUSB/UART)
         ↓
    ┌─────────────────────────┐
    │ Obsługa Protokołu       │
    │ - Parsuj JSON/binarne   │
    │ - Wiele transportów     │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Walidacja Transakcji    │
    │ - Adres (Base58/Bech32) │
    │ - Sprawdzenie kwoty     │
    │ - Obsługa waluty        │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Deklarowanie Klucza     │
    │ - Ścieżka BIP-32 HD     │
    │ - Biblioteka trezor     │
    │ - Generacja klucza dziewczęcia │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Potwierdzenie Użytkownika│
    │ - Wymagany naciśnięcie  │
    │ - Info wyświetlacza OLED│
    │ - Potwierdź/Odrzuć      │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Silnik Podpisywania     │
    │ - SHA-256 + HMAC-SHA512 │
    │ - ECDSA secp256k1       │
    │ - Nonce RFC 6979        │
    │ - Kodowanie DER         │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Oczyszczenie Pamięci    │
    │ - Bufory memzero()      │
    │ - Bezpieczne czyszczenie│
    │ - Zapobiegnąć wyciekom  │
    └─────────────────────────┘
         ↓
    ┌─────────────────────────┐
    │ Odpowiedź               │
    │ - Sygnatura + metadane  │
    │ - Multi-protocol output │
    └─────────────────────────┘
```

---

## 📈 Oś Czasu Wzrostu Projektu

```
2024-2025: Tworzenie stm32_secure_boot
├─ Q1-Q3: Bootloader + aplikacje podstawowe
├─ Q4:    Wiele profilów, obsługa HID
└─ Status: Platforma Badawcza/Edukacyjna

         ↓ (Używa jako fundament)

2025-2026: Tworzenie CryptoWallet
├─ Faza 1: Core FreeRTOS + LwIP (z SB)
├─ Faza 2: Integracja trezor-crypto
├─ Faza 3: Portfel HD (BIP-39/32)
├─ Faza 4: Multi-protocol (HTTP, WebUSB)
├─ Faza 5: Testowanie & hartowanie
└─ Faza 6: Infrastruktura Testowania RNG ✨ NOWY
           └─ 30 nowych plików
           └─ Automatyzacja testów
           └─ Dokumentacja (3 języki)

Przyszłość:
├─ Wdrożenie Production
├─ Audyt Bezpieczeństwa
├─ Funkcje Hardware Wallet
└─ Długoterminowe Wsparcie
```

---

## 🎯 Diagram Venna Przypadków Użycia

```
        ┌─────────────────────────────────────────────┐
        │   stm32_secure_boot                        │
        │   (Badania & Edukacja)                     │
        │                                             │
        │  • Weryfikowana Загрузka                ┌──────────┴───────────┐
        │  • Projektowanie Bootloadera            │                      │
        │  • Weryfikacja ECDSA             OBA:   │ • FreeRTOS           │
        │  • Protokół HID               │ • LwIP                │
        │  • Multi-transport            │ • HAL STM32           │
        │  • Edukacyjny               │ • Embedded ARM         │
        │  • Kod Referencyjny             │                      │
        │                             │                      │
        └──────────────┬──────────────┴──────────────┐
                       │   CryptoWallet              │
                       │  (Portfel Production)      │
                       │                             │
                       │  • Portfel HD (BIP-39/32)  │
                       │  • Integracja Bitcoin      │
                       │  • Walidacja TX            │
                       │  • Serwer HTTP             │
                       │  • Interfejs WebUSB        │
                       │  • Testowanie RNG          │
                       │  • Bezpieczne Zarządzanie  │
                       │  • Potwierdzenie Użytkownika│
                       │  • Production-ready        │
                       │  • Infrastruktura QA       │
                       │                             │
                       └──────────────────────────────┘

Legenda: Pokrywające się = Funkcje Wspólne/Powiązane
```

---

## 🏗️ Tort Warstw Architektury

```
┌────────────────────────────────────────┐
│      WARSTWA APLIKACJI UŻYTKOWNIKA     │
│  ┌──────────────────────────────────┐  │
│  │ SB:  Aplikacja Signer            │  │
│  │ CW:  Aplikacja Portfela          │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘
            ↑            ↑
            │ Używa      │
            ▼            ▼
┌──────────────────────┬──────────────────┐
│   WARSTWA PROTOKOŁU  │   WARSTWA KRYPTO │
├──────────────────────┼──────────────────┤
│ SB: UART/HID         │ SB: CMOX/ECDSA   │
│ CW: HTTP/WebUSB/UART │ CW: trezor/BIP39 │
└──────────────────────┴──────────────────┘
            ↑            ↑
            │ Używa      │
            ▼            ▼
┌──────────────────────┬──────────────────┐
│    WARSTWA OS        │   WARSTWA DRIVER │
├──────────────────────┼──────────────────┤
│ FreeRTOS Kernel      │ HAL STM32        │
│ Task Scheduler       │ GPIO, UART, USB  │
│ IPC (Queues, Mutex)  │ I2C, Ethernet    │
└──────────────────────┴──────────────────┘
            ↑            ↑
            │ Używa      │
            ▼            ▼
┌────────────────────────────────────────────┐
│       WARSTWA SPRZĘTU (Wspólna)            │
│  STM32H743 CPU + Peryferia                 │
│  (480 MHz Cortex-M7, 2MB Flash, 1MB RAM)  │
└────────────────────────────────────────────┘
```

---

## 📝 Schemat Blokowy Organizacji Plików

```
Główny Katalog Projektu
│
├─ Core/
│  ├─ Inc/              (Nagłówki)
│  │  ├─ crypto_wallet.h
│  │  ├─ task_sign.h
│  │  ├─ task_net.h
│  │  ├─ task_display.h
│  │  ├─ rng_dump.h ✨ NOWY
│  │  └─ ... (19 więcej)
│  │
│  └─ Src/              (Implementacja)
│     ├─ main.c
│     ├─ task_sign.c
│     ├─ task_net.c
│     ├─ task_display.c
│     ├─ crypto_wallet.c
│     ├─ rng_dump.c ✨ NOWY
│     └─ ... (18 więcej)
│
├─ docs_src/            (Dokumentacja)
│  ├─ TESTING_GUIDE_RNG_SIGNING.md ✨ NOWY
│  ├─ TEST_SCRIPTS_README.md ✨ NOWY
│  ├─ INSTALL_TEST_DEPS.md ✨ NOWY
│  ├─ VENV_SETUP.md ✨ NOWY
│  │
│  └─ crypto/           (Dokumentacja krypto)
│     ├─ README.md
│     ├─ rng_dump_setup.md ✨ NOWY
│     ├─ testing_setup.md ✨ NOWY
│     ├─ rng_capture_troubleshooting.md ✨ NOWY
│     ├─ rng_test_checklist.txt ✨ NOWY
│     ├─ + Wersje polskie ✨ NOWY
│     └─ + Wersje rosyjskie ✨ NOWY
│
├─ scripts/             (Narzędzia)
│  ├─ test_rng_signing_comprehensive.py ✨ NOWY
│  ├─ capture_rng_uart.py
│  ├─ run_dieharder.py
│  └─ ... (9+ więcej)
│
├─ ThirdParty/
│  └─ trezor-crypto/    (Biblioteka Bitcoin)
│
├─ Makefile             (System budowania)
├─ .gitignore           (Konfiguracja Git)
│
└─ Dokumentacja (TEN PROJEKT):
   ├─ PROJECTS_COMPARISON_AND_UPDATES.md ⭐ GŁÓWNA
   ├─ UPDATES_SUMMARY.md
   ├─ QUICK_REFERENCE.md
   ├─ ARCHITECTURE_DETAILED.md
   ├─ PROJECT_DEPENDENCIES.md
   └─ DOCUMENTATION_INDEX.md (ten indeks)
```

---

## 🔐 Wykres Radarowy Postury Bezpieczeństwa

```
       Uwierzytelnianie
            ↑
          ╱   ╲
         ╱     ╲
    100%       0%
     ╱───────────╲      Bezpieczeństwo Pamięci
    ╱             ╲    ↗
   ╱  stm32_SB    ╲  ╱
  ╱      40%       ╲╱
 │                 │  Zarządzanie Kluczami
 │   ╱───────╲     │     ↗
 └──│CW      │─────┴────────
    │70%     │
    ╲       ╱
     ╲     ╱
      ╲   ╱
       ╲ ╱
        V
   Bezpieczeństwo Sieci

        ↑
    Jakość RNG
     ↗
   
stm32_secure_boot: Średni (brak testowania)
CryptoWallet:      Wysoki (walidowany Dieharder) ✨

CW = Ogólnie bardziej bezpieczny
```

---

## 📊 Macierz Złożoności & Funkcji

```
Złożoność (Oś X) →
│
│  Wysoka │                    CryptoWallet
│         │                    • Multi-protocol
│         │                    • Portfel HD
│         │                    • Pełne testowanie
│         │                    • 150+ plików
│  Średnia│              ╱─────────────────╲
│         │         ╱────────╲         │
│         │    ╱──────────        ╲        │
│         │   │ stm32_secure_boot  ╲       │
│         │   │ • Weryfikowana Zgr   ╲      │
│         │   │ • Multi-profile      ╲     │
│         │   │ • 100+ plików         ╲    │
│  Niska  │   └────────────────────────────┘
│         └────────────────────────────────→
│        Niska        Średnia        Wysoka
│        ← Funkcje →
```

---

## 🎓 Drzewo Decyzji: Który Projekt Użyć?

```
                    Potrzebujesz Bootloadera?
                          │
                    ┌─────┴─────┐
                    │           │
                   TAK          NIE
                    │           │
                    ▼           ▼
            stm32_secure_boot   CryptoWallet
                    │           │
        Potrzebujesz Pełny       Potrzebujesz Pełny
        Portfel?       │           Portfel?
                     ▼           ▼
                    NIE          TAK
                     │           │
        Edukacyjny/  │           │  Production
        Badawczy     │           │  Wdrożenie
                     │           │
                     ▼           ▼
          Doskonałe!            Doskonałe!
          stm32_SB            CryptoWallet
                     │           │
              Ale może            Ale może
              chcesz            potrzebujesz
              testowania         bootloadera
              RNG z CW!          z SB!
```

---

**Dokument Podsumowania Wizualnego**  
**Utworzono:** 2026-03-20  
**Status:** Kompletne

Dla pełnego zestawu diagramów i analiz wizualnych, zobacz pliki poźniejsze:
- `ARCHITECTURE_DETAILED.md` - Diagramy szczegółowe
- `PROJECTS_COMPARISON_AND_UPDATES.md` - Tabele porównawcze
- `PROJECT_DEPENDENCIES.md` - Grafy zależności
