# Architektura & Stos Technologiczny - Porównanie

**Wizualny Przewodnik: stm32_secure_boot vs CryptoWallet**

---

## 🏛️ Porównanie Architektury Systemu

### Architektura stm32_secure_boot

```
┌─────────────────────────────────────────────────────────────────┐
│                     WARSTWA SPRZĘTU                             │
│  STM32H743 (Cortex-M7 @ 480 MHz, 2MB Flash, 1MB SRAM)         │
│  Peryferia: UART, USB, I2C, GPIO, RNG (opcjonalny), Ethernet   │
└─────────────────────────────────────────────────────────────────┘

                    ↓ Sekwencja загрузki ↓

┌─────────────────────────────────────────────────────────────────┐
│          BOOTLOADER (64 KB @ 0x08000000)                        │
│                                                                  │
│  ┌─ Kod startowy (asembler ARM Cortex-M7)                     │
│  ├─ Konfiguracja zegara (PLL, dzielniki AHB)                  │
│  ├─ Inicjalizacja HAL                                          │
│  ├─ Obliczenia SHA-256 (Hash Obrazu)                          │
│  │   └─ hash = SHA256(app_image)                              │
│  ├─ Weryfikacja Sygnatury ECDSA                               │
│  │   ├─ Z biblioteka CMOX (rzeczywista krypto)               │
│  │   └─ Lub tryb STUB (do testowania)                        │
│  ├─ Decyzja Weryfikacji                                        │
│  │   ├─ JEŚLI sygnatura prawidłowa → LED zielone, skok do app│
│  │   └─ JEŚLI sygnatura błędna → LED czerwone, zatrzymaj     │
│  └─ Skok do Aplikacji                                          │
│       └─ jmp 0x08010000 (Adres Aplikacji)                    │
└─────────────────────────────────────────────────────────────────┘

                    ↓ Zweryfikowany ↓

┌─────────────────────────────────────────────────────────────────┐
│          APLIKACJA (256+ KB @ 0x08010000)                       │
│                                                                  │
│  ╔══════════════════════════════════════════════════════════╗  │
│  ║           KERNEL FreeRTOS                               ║  │
│  ║  Scheduler, Tasks, Queues, Mutexes, Semaphores         ║  │
│  ╚══════════════════════════════════════════════════════════╝  │
│                                                                  │
│  Task 1: FSM Podpisywania         Task 2: UI/Wyświetlacz      │
│  ├─ Odbierz UART                  ├─ I2C (SSD1306)           │
│  ├─ Parsuj polecenia              ├─ 128x32 pikseli          │
│  ├─ Deklarowanie kluczy            └─ Rendering tekstu 4 linii
│  ├─ Obliczenia SHA-256                                       │
│  ├─ Podpisz ECDSA                 Task 3: Wejście Przycisku  │
│  ├─ Kolejka odpowiedzi            ├─ Debouncing (30ms)      │
│  └─ Następny stan                 └─ Potwierdź/Odrzuć       │
│                                                             │
│  Opcjonalnie: LwIP (lwip_zero)    Opcjonalnie: USB HID      │
│  ├─ Ethernet                      ├─ Raporty 64-bajtowe     │
│  ├─ Stos IP                       ├─ Raport IN/OUT          │
│  └─ Serwer HTTP                   └─ Protokół aplikacji     │
└─────────────────────────────────────────────────────────────────┘
```

### Architektura CryptoWallet

```
┌─────────────────────────────────────────────────────────────────┐
│                     WARSTWA SPRZĘTU                             │
│  STM32H743 (Cortex-M7 @ 480 MHz, 2MB Flash, 1MB SRAM)         │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ UART (USART3)    │  │ USB (Device)     │  │ Ethernet     │ │
│  │ 115200 baud      │  │ FS @ 12 Mbps     │  │ Tryb RMII    │ │
│  │ Debug/Control    │  │ Protokół WebUSB  │  │ Klient DHCP  │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ I2C (I2C1)       │  │ SPI (jeśli trzeba)│ │ RNG (AES)    │ │
│  │ OLED SSD1306     │  │ Przechowywanie   │  │ Słowa 32-bit │ │
│  │ Wyświetlacz 128×32│ │ flash (opcjonalnie) │ Statystyczne │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ GPIO: Przycisk (PC13), LEDy (wskaźnik)                    │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Firmware ↓

┌─────────────────────────────────────────────────────────────────┐
│               BOOTLOADER (Zewnętrzny lub Wewnętrzny)            │
│  Opcjonalnie: SHA-256 + weryfikacja ECDSA (z stm32_secure_boot)
└─────────────────────────────────────────────────────────────────┘

                        ↓ Boot ↓

┌─────────────────────────────────────────────────────────────────┐
│                    WARSTWA INICJALIZACJI SPRZĘTU               │
│  hw_init.c                                                      │
│  ├─ Konfiguracja zegara (PLL, HSI48)                          │
│  ├─ Setup GPIO (TX/RX UART, SDA/SCL I2C, USB, Ethernet)     │
│  ├─ Inicjalizacja UART (USART3 @ 115200)                     │
│  ├─ Inicjalizacja I2C (100 kHz dla OLED)                     │
│  ├─ Inicjalizacja USB device (FS @ 12 Mbps)                  │
│  ├─ Inicjalizacja Ethernet PHY (RMII)                        │
│  ├─ Inicjalizacja RNG (jeśli USE_RNG_DUMP=1) ⭐ NOWY        │
│  └─ Setup NVIC (priorytety przerwań)                         │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Utwórz OS ↓

┌─────────────────────────────────────────────────────────────────┐
│             KERNEL FreeRTOS + OBIEKTY IPC                       │
│                                                                  │
│  ╔══════════════════════════════════════════════════════════╗  │
│  ║         Scheduler & Kernel FreeRTOS                     ║  │
│  ║  - Tworzenie zadań                                      ║  │
│  ║  - Context switching (co 1 ms SysTick)                ║  │
│  ║  - Zarządzanie kolejkami                              ║  │
│  ║  - Obsługa mutex/semafora                            ║  │
│  ╚══════════════════════════════════════════════════════════╝  │
│                                                                  │
│  Kolejki:                                                       │
│  ├─ tx_request_queue    (HTTP/USB → Zadanie Sign)            │
│  ├─ sign_response_queue  (Zadanie Sign → Zadanie Net)        │
│  ├─ display_queue        (Wszystkie → Aktualizacja Display)  │
│  └─ uart_log_queue       (Wszystkie → Wyjście UART)          │
│                                                                  │
│  Mutex: crypto_lock (chroni bibliotekę trezor-crypto)         │
│  Events: user_confirm_event (naciśnięcie przycisku)           │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Utwórz Zadania ↓

┌─────────────────────────────────────────────────────────────────┐
│                    ZADANIA PROTOKOŁU/STEROWNIKA                 │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zadanie Sieci (task_net.c) - Priorytet: 20             ║ │
│  ║  Serwer HTTP (port 80) przez LwIP                      ║ │
│  ║  ├─ Odbierz: POST /tx (JSON lub dane formularza)      ║ │
│  ║  ├─ Parsuj:  {amount, address, currency}              ║ │
│  ║  ├─ Enqueue: tx_request → tx_request_queue             ║ │
│  ║  └─ Wyślij: HTTP 200 OK lub błąd                       ║ │
│  ║                                                        ║ │
│  ║  Interfejs WebUSB (usb_webusb.c)                       ║ │
│  ║  ├─ Bulk IN: Odpowiedź sygnatury                       ║ │
│  ║  └─ Bulk OUT: Żądanie TX                              ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zadanie Podpisywania (task_sign.c) - Priorytet: 21   ║ │
│  ║  FSM podpisywania oparte na IPC ⭐ Komponent Kluczowy ║ │
│  ║  ├─ Czekaj: na tx_request_queue                        ║ │
│  ║  ├─ Waliduj: adres (Base58/bech32)                    ║ │
│  ║  ├─ Deklaruj: klucz z seed (BIP-32)                   ║ │
│  ║  ├─ Czekaj: potwierdzenie użytkownika (przycisk)      ║ │
│  ║  ├─ Podpisz: SHA-256 + ECDSA (trezor-crypto)          ║ │
│  ║  ├─ Wyzeruj: wrażliwe bufory (memzero)                ║ │
│  ║  ├─ Wyślij: sygnaturę → sign_response_queue           ║ │
│  ║  └─ Pętla                                              ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zadanie Wyświetlacza (task_display.c) - Priorytet: 15  ║ │
│  ║  Zarządzanie SSD1306 OLED (128×32 pikseli)             ║ │
│  ║  ├─ Render: 4-liniowy tekst + przewijany dziennik      ║ │
│  ║  ├─ Stan: "Czekanie...", "Podpisywanie...", "OK", "ERR"║ │
│  ║  ├─ Aktualizuj: @ 10 Hz (co 100 ms)                   ║ │
│  ║  └─ Czytaj: display_queue dla aktualizacji             ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zadanie Użytkownika (task_user.c) - Priorytet: 22    ║ │
│  ║  (najwyższy) Obsługiwacz Wejścia Przycisku & Klawiatury║ │
│  ║  ├─ Poll: przycisk PC13 (interwały 20 ms)             ║ │
│  ║  ├─ Debounce: wstabilna detekcja 30 ms                ║ │
│  ║  ├─ Sygnał: user_confirm_event → Zadanie Podpisywania ║ │
│  ║  └─ LED: informacja zwrotna statusu                    ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zadanie IO (task_io.c) - Priorytet: 18                ║ │
│  ║  Zarządzanie Wskaźnikiem LED                            ║ │
│  ║  ├─ Zielony: Normalny stan                             ║ │
│  ║  ├─ Czerwony: Stan błędu                               ║ │
│  ║  ├─ Niebieski: Aktywność sieci                        ║ │
│  ║  └─ Wzory migania: Wskazanie statusu                   ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  Opcjonalne Zadanie RNG (NOWE) ⭐                             │
│  ├─ Czytaj: Peryferia RNG @ 6 MHz                           │
│  ├─ Wyjście: UART @ 115200                                   │
│  └─ Użyj: Testowanie statystyczne Dieharder                 │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Stosy Protokołów ↓

┌─────────────────────────────────────────────────────────────────┐
│         WARSTWY KOMUNIKACJI PROTOKOŁU                           │
│                                                                  │
│  Stos LwIP (Ethernet)                                           │
│  ├─ MAC: Interfejs RMII (ETH1/2)                              │
│  ├─ IP: DHCP lub statyczne                                    │
│  ├─ TCP/UDP: Pełny stos                                       │
│  └─ HTTP: Prosty serwer (port 80)                            │
│                                                                  │
│  Stos USB                                                       │
│  ├─ Urządzenie FS (12 Mbps)                                   │
│  ├─ CDC ACM: Serial wirtualny (opcjonalny)                   │
│  ├─ WebUSB: Niestandardowe punkty końcowe bulk ⭐           │
│  └─ Deskryptory: BOS Możliwość Platformy                     │
│                                                                  │
│  Protokół UART                                                  │
│  └─ Komunikacja debug/fallback                                │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Kryptografia ↓

┌─────────────────────────────────────────────────────────────────┐
│          WARSTWA KRYPTOGRAFII (trezor-crypto)                  │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Zarządzanie Kluczami                                    ║ │
│  ║  Przechowywanie Seed:                                   ║ │
│  ║  ├─ Opcja 1: Seed testowy (zakodowany na deweloper)   ║ │
│  ║  ├─ Opcja 2: Mnemoninik BIP-39 (12 lub 24 słowa)      ║ │
│  ║  └─ Przechowywane w RAM (wyczyszczone przy wyłączeniu) ║ │
│  ║                                                        ║ │
│  ║  Deklarowanie Klucza (BIP-32):                         ║ │
│  ║  ├─ Klucz główny z seed                                ║ │
│  ║  ├─ Ścieżka: m/44'/0'/0'/0/0 (standard BIP-44)        ║ │
│  ║  ├─ Deklaruj: pochodne klucze publiczne/prywatne       ║ │
│  ║  └─ Dla każdego adresu klucze (nie ponownie)          ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Haszowanie                                              ║ │
│  ║  ├─ SHA-256: Skrót transakcji                           ║ │
│  ║  ├─ HMAC-SHA-512: Deklarowanie kluczy BIP-32          ║ │
│  ║  └─ RIPEMD-160: Generacja adresu (Bitcoin)            ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Podpisywanie (ECDSA)                                    ║ │
│  ║  ├─ Krzywa: secp256k1 (standard Bitcoin)                ║ │
│  ║  ├─ Hash: SHA-256(transaction)                         ║ │
│  ║  ├─ Nonce: Generowany na sygnaturę (parametr k)       ║ │
│  ║  ├─ Sygnatura: para (r, s)                            ║ │
│  ║  └─ Kodowanie: Format DER                              ║ │
│  ║                                                        ║ │
│  ║  Właściwości Bezpieczeństwa:                           ║ │
│  ║  ├─ Deterministyczne (RFC 6979) lub losowe             ║ │
│  ║  ├─ Podatne jeśli k się kompromituje                   ║ │
│  ║  └─ Dlatego: Jakość RNG krytyczna ⭐ NOWY             ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
│                                                                  │
│  ╔═══════════════════════════════════════════════════════════╗ │
│  ║ Bezpieczeństwo Pamięci                                  ║ │
│  ║  ├─ memzero(): Bezpieczne czyszczenie bufora            ║ │
│  ║  ├─ Zapobiega optymalizacji kompilatora                 ║ │
│  ║  ├─ Zastosowany do: seedów, kluczy, podpisów          ║ │
│  ║  └─ Krytyczne dla zmniejszenia wycieków kluczy         ║ │
│  ╚═══════════════════════════════════════════════════════════╝ │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Walidacja ↓

┌─────────────────────────────────────────────────────────────────┐
│         WARSTWA WALIDACJI TRANSAKCJI                            │
│                                                                  │
│  Wejście: Żądanie TX Bitcoin {amount, address, currency}      │
│                                                                  │
│  Kroki Walidacji:                                               │
│  1. Sprawdzenie Formatu Adresu                                 │
│     ├─ Base58Check (P2PKH: 1...)                              │
│     ├─ Bech32 (SegWit: bc1...)                               │
│     └─ Zwróć błąd jeśli nieprawidłowy                         │
│                                                                  │
│  2. Walidacja Kwoty                                            │
│     ├─ Sprawdzenie zakresu (> 0, < 21M BTC)                 │
│     ├─ Szacowanie opłaty                                      │
│     └─ Zwróć błąd jeśli nieprawidłowy                         │
│                                                                  │
│  3. Sprawdzenie Waluty                                         │
│     ├─ Obsługa: BTC (mainnet/testnet)                        │
│     └─ Zwróć błąd jeśli niewspierane                         │
│                                                                  │
│  Wyjście: Zwalidowana TX lub komunikat błędu                  │
└─────────────────────────────────────────────────────────────────┘

                        ↓ Wyjście ↓

┌─────────────────────────────────────────────────────────────────┐
│              WARSTWA INTERFEJSU UŻYTKOWNIKA                     │
│                                                                  │
│  SSD1306 OLED (128×32 pikseli)                                 │
│  ┌────────────────────────────────────────┐                    │
│  │ CryptoWallet v1.0                  [OK]│                    │
│  │ Podpisywanie tx...                     │                    │
│  │ Kwota: 1.5 BTC → 1A1z...             │                    │
│  │ Potwierdź? (Naciśnij przycisk)         │                    │
│  └────────────────────────────────────────┘                    │
│                                                                  │
│  Przycisk (PC13)                                                │
│  └─ Potwierdź: Naciśnij aby podpisać                          │
│  └─ Odrzuć: Przytrzymaj na 2 sek                              │
│                                                                  │
│  Status LED                                                     │
│  ├─ Zielony (500 ms): Gotowy                                  │
│  ├─ Niebieski (50 ms): Aktywność sieci                       │
│  ├─ Czerwony (100 ms): Błąd                                   │
│  └─ Wyłączony: Oszczędzanie energii                           │
│                                                                  │
│  Wyjście UART (debug)                                          │
│  └─ Cała aktywność zalogowana dla rozwiązywania problemów    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📊 Macierz Stosu Technologicznego

|| Warstwa | stm32_secure_boot | CryptoWallet |
||---|---|---|
|| **CPU** | STM32H743 (Cortex-M7, 480 MHz) | STM32H743 (Cortex-M7, 480 MHz) |
|| **OS** | FreeRTOS (8.2.3 lub nowszy) | FreeRTOS (8.2.3+) |
|| **Pamięć** | 2 MB Flash / 1 MB SRAM | 2 MB Flash / 1 MB SRAM |
|| **Budowanie** | GNU Make + arm-none-eabi-gcc | GNU Make + arm-none-eabi-gcc |
|| **Bootloader** | ✅ SHA-256 + ECDSA (CMOX) | ❌ (opcjonalny) |
|| **Stos Sieciowy** | LwIP (lwip_zero) | LwIP (obowiązkowy) |
|| **Biblioteka Krypto** | CMOX (ST Micro) | trezor-crypto (Satoshi Labs) |
|| **Portfel HD** | ❌ | ✅ BIP-39/BIP-32 |
|| **USB** | ✅ HID (64-bajtowe raporty) | ✅ WebUSB (punkty końcowe bulk) |
|| **Protokół** | Podwójny (UART + HID) | Potrójny (UART + WebUSB + HTTP) |
|| **Wyświetlacz** | Opcjonalny SSD1306 | Obowiązkowy SSD1306 |
|| **Testowanie RNG** | ❌ | ✅ Zestaw testów Dieharder (NOWY) |
|| **Dokumentacja** | Polski/Rosyjski | Angielski/Rosyjski/Polski |

---

## 🔗 Porównanie Protokołów Komunikacji

### stm32_secure_boot - Transport UART

```
Format Polecenia Hosta:
┌─────────────────────────────────┐
│ Nagłówek Ramki: "CMD:"          │
│ Polecenie: PING, PONG, STATUS...│
│ Parametry: [opcjonalnie]        │
│ Terminator: \r\n               │
└─────────────────────────────────┘

Przykład:
  Host → Urządzenie: "CMD:SIGN\r\n"
  Urządzenie → Host: "PONG:OK\r\n"
```

### CryptoWallet - Protokół HTTP

```
POST /tx HTTP/1.1
Host: 192.168.1.100
Content-Type: application/x-www-form-urlencoded

amount=1.5&address=1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa&currency=BTC

HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "signed",
  "signature": "304402201f2...",
  "tx_id": "abc123def456..."
}
```

### CryptoWallet - Protokół WebUSB

```
Format Ramki WebUSB:
┌──────────────────────────────────┐
│ Nagłówek (4 bajty): typ ramki    │
│ Długość (2 bajty): długość ładunku │
│ Ładunek (0-58 bajtów): dane      │
│ CRC (2 bajty): sprawdzenie błędu │
└──────────────────────────────────┘

Typowy przepływ:
1. Host: Ramka Ping
2. Urządzenie: Odpowiedź Pong
3. Host: Żądanie TX (JSON)
4. Urządzenie: Odpowiedź Sygnatury
```

---

**Dokument:** Architektura & Technologia  
**Ostatnia Aktualizacja:** 2026-03-20  
**Status:** Kompletne

(Pełny dokument zawiera również sekcje dotyczące layoutu pamięci, szczegóły na temat warstwowania kryptografii i pełne schematy danych - zob. wersję angielską dla pełnych detali)
