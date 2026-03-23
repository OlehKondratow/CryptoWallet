# 1. Model zaufania i architektura

## 1.1 Obraz systemu

CryptoWallet to **oprogramowanie aplikacyjne** na STM32H743. Nie zastępuje ROM, bajtów opcji ani zweryfikowanego bootloadera. Twierdzenia o bezpieczeństwie należy podawać **warstwowo**:

```
STM32 ROM  →  (opcjonalnie) zweryfikowany bootloader @ stm32_secure_boot  →  CryptoWallet we Flash
```

| Warstwa | Repozytorium / właściciel | Co udowadnia |
|---------|---------------------------|--------------|
| ROM / blokada debug | ST, konfiguracja płyty | Dostęp debug, RDP, wektory |
| Bootloader | `stm32_secure_boot` (osobne repozytorium) | Autentyczność obrazu aplikacji **przed** skokiem do app |
| Aplikacja | **to repozytorium** | Logika portfela, samokontrole w czasie działania, powierzchnie UART/HTTP/USB |

Wszystko, co wymaga **kryptografii ROM lub bootloadera**, jest **poza zakresem** dokumentacji CWUP i HTTP w tym repozytorium — działają one dopiero **po** `main()` i starcie schedulera.

## 1.2 Odpowiedzialności aplikacji (to repozytorium)

- **Podpisywanie transakcji** z potwierdzeniem użytkownika na urządzeniu (przycisk), nie przez sieć.
- **Opcjonalna integralność firmware** (`fw_integrity`): CRC/długość obszaru aplikacji — wykrywa przypadkowe uszkodzenia; **to nie** verified boot.
- **Diagnostyka:** UART CWUP (gdy `USE_RNG_DUMP=0`), HTTP (port 80), WebUSB — każda ma inne założenia zaufania (rozdziały 4–6).

## 1.3 Powierzchnie (atak / operator)

| Powierzchnia | Poufność | Integralność | Uwagi |
|--------------|----------|--------------|-------|
| HTTP :80 | Brak (brak TLS) | Parsowanie wg możliwości | Tylko zaufana LAN; potwierdzenie na OLED |
| WebUSB | Zależy od przeglądarki i kabla | Ramkowy protokół binarny | Ścieżka produktowa obok HTTP |
| UART3 CWUP | Brak szyfrowania | Linie AT | Lab/CI; **brak** uwierzytelnienia dla `AT+WALLET?` itd. |
| UART3 `USE_RNG_DUMP=1` | N/D | Surowe bajty do statystyk | **Wyłącza** CWUP na tym UART |

## 1.4 Kształt FreeRTOS

Po `HW_Init()` i `osKernelStart()` typowe zadania to: wyświetlacz, sieć (LwIP + HTTP), podpis, IO (LED/przycisk), wejście użytkownika, opcjonalnie USB, CWUP, zrzut RNG.

**IPC** (kolejki, grupy zdarzeń, mutexy) łączy ingress sieci/WebUSB z podpisem i wyświetlaczem. Dokładne uchwyty są w `wallet_shared.h` i `main.c`.

## 1.5 Flagi kompilacji zmieniające postawę bezpieczeństwa

| Flaga | Skutek |
|-------|--------|
| `USE_CRYPTO_SIGN` | Pełna ścieżka trezor-crypto vs minimalna tylko skrót |
| `USE_TEST_SEED` | **Znana mnemonic** — tylko rozwój, nigdy prawdziwe środki |
| `USE_RNG_DUMP` | Binarny TRNG na UART; **brak** CWUP na tym UART |
| `USE_WEBUSB` | Urządzenie USB + deskryptory WebUSB |
| `USE_LWIP` | Ethernet + serwer HTTP |

## 1.6 Relacja do `stm32_secure_boot`

- Podpisywanie i weryfikacja bootloadera są udokumentowane tam (np. `sign_image.py`, układ obrazu).
- Testy po stronie hosta w tym repozytorium (`secure_boot_image.py`, `tests/mvp/…`) sprawdzają logikę podpisu/manipulacji obrazem **jako dane**, nie wykonanie z ROM.
- Polecenie CWUP `AT+BOOTCHAIN?` jest **informacyjne** (makra, `VTOR`); nie dowodzi sprzętowego root of trust.
