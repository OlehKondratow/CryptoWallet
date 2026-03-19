# CryptoWallet

Aplikacja mikrokontrolera do bezpiecznego podpisywania transakcji Bitcoin na STM32H743. Integruje trezor-crypto, FreeRTOS, LwIP, UI SSD1306 i WebUSB.

**Dokumentacja dostępna w:**
- 🇬🇧 [English](README.md)
- 🇷🇺 [Русский](README_ru.md)
- 🇵🇱 [Polski](README_pl.md) (ten plik)

---

## 🔐 Core & Security (Jądro i Bezpieczeństwo)

Główna logika podpisywania, zarządzanie kluczami, walidacja i operacje kryptograficzne.

### [main.c](Core/Src/main.c) — Punkt wejścia i zarządzanie aplikacją
Punkt wejścia FreeRTOS: inicjalizacja obiektów IPC (kolejki, semafory, grupy zdarzeń), tworzenie i uruchamianie zadań krytycznych.
**Pełna dokumentacja:** [docs_src/main.md](docs_src/main.md)

### [task_sign.c](Core/Src/task_sign.c) — Pipeline podpisywania (FSM)
Główna funkcja: walidacja żądania z kolejki, utworzenie SHA-256, oczekiwanie na potwierdzenie użytkownika, podpis ECDSA, zapisanie wyniku.
**Pełna dokumentacja:** [docs_src/task_sign.md](docs_src/task_sign.md)

### [crypto_wallet.c](Core/Src/crypto_wallet.c) — Warstwa kryptograficzna (trezor-crypto)
Wrapper nad trezor-crypto: STM32 RNG + mieszanie entropii, mnemoniki BIP-39, pochodna HD BIP-32 (m/44'/0'/0'/0/0), ECDSA secp256k1.
**Pełna dokumentacja:** [docs_src/crypto_wallet.md](docs_src/crypto_wallet.md)

### [tx_request_validate.c](Core/Src/tx_request_validate.c) — Brama walidacji
Warstwa ochronna przed podpisywaniem: walidacja adresu (Base58/bech32), kwoty (decimal), waluty (whitelist).
**Pełna dokumentacja:** [docs_src/tx_request_validate.md](docs_src/tx_request_validate.md)

### [memzero.c](Core/Src/memzero.c) — Bezpieczne zerowanie
Niszczenie wrażliwych buforów (klucze, skróty, ziarna) za pośrednictwem zapisów volatile, ochrona przed optymalizacją kompilatora.
**Pełna dokumentacja:** [docs_src/memzero.md](docs_src/memzero.md)

### [sha256_minimal.c](Core/Src/sha256_minimal.c) — Rezerwa SHA-256
Kompaktna implementacja SHA-256 (gdy `USE_CRYPTO_SIGN=0` bez trezor-crypto).
**Pełna dokumentacja:** [docs_src/sha256_minimal.md](docs_src/sha256_minimal.md)

### [wallet_seed.c](Core/Src/wallet_seed.c) — Zarządzanie ziarnem (test)
Ziarno testowe do programowania (`USE_TEST_SEED=1`): wektor BIP-39 "abandon...about", **tylko do programowania**.
**Pełna dokumentacja:** [docs_src/wallet_seed.md](docs_src/wallet_seed.md)

### [task_security.c](Core/Src/task_security.c) — Starsze podpisywanie (audit/test)
Alternatywny FSM podpisywania z fikcyjną kryptografią do bring-up i porównania.
**Pełna dokumentacja:** [docs_src/task_security.md](docs_src/task_security.md)

**Nagłówki:** crypto_wallet.h, memzero.h, sha256_minimal.h, task_sign.h, task_security.h, tx_request_validate.h, wallet_shared.h

---

## 📡 Communication Interfaces (Interfejsy komunikacji)

Stos sieciowy (LwIP/Ethernet), USB (WebUSB), synchronizacja czasu.

### [task_net.c](Src/task_net.c) — Serwer HTTP i API sieciowy
Uruchomienie LwIP/Ethernet, HTTP na porcie 80, parsowanie JSON/form `POST /tx`, walidacja, enqueue do `g_tx_queue`.
**Pełna dokumentacja:** [docs_src/task_net.md](docs_src/task_net.md)

### [usb_webusb.c](Core/Src/usb_webusb.c) — Interfejs dostawcy WebUSB
WebUSB właściwy dla dostawcy: punkty końcowe zbiorcze, ping/pong, rama binarna do żądania podpisu.
**Pełna dokumentacja:** [docs_src/usb_webusb.md](docs_src/usb_webusb.md)

### [app_ethernet_cw.c](Src/app_ethernet_cw.c) — Połączenie Ethernet i FSM DHCP
Callback połączenia Ethernet dla up/down, maszyna stanu DHCP (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), sprzężenie zwrotne LED.
**Pełna dokumentacja:** [docs_src/app_ethernet_cw.md](docs_src/app_ethernet_cw.md)

### [time_service.c](Core/Src/time_service.c) — SNTP i UTC
Synchronizacja czasu za pośrednictwem SNTP, ujednolicony dostęp do epoki Unix i ciągów UTC.
**Pełna dokumentacja:** [docs_src/time_service.md](docs_src/time_service.md)

### [usb_device.c](Core/Src/usb_device.c) — Inicjalizacja urządzenia USB
Konfiguracja zegara HSI48, inicjalizacja rdzenia USBD, rejestracja klasy WebUSB.
**Pełna dokumentacja:** [docs_src/usb_device.md](docs_src/usb_device.md)

### [usbd_conf_cw.c](Core/Src/usbd_conf_cw.c) — Konfiguracja BSP USB
Statyczny alokatora dla USBD, MSP dla PCD (GPIO AF, zegar, NVIC), łączenie HAL_PCD do USBD_LL.
**Pełna dokumentacja:** [docs_src/usbd_conf_cw.md](docs_src/usbd_conf_cw.md)

### [usbd_desc_cw.c](Core/Src/usbd_desc_cw.c) — Deskryptory USB
Deskryptory Device/interface/BOS, ciągi (producent, produkt, seria), UUID WebUSB Platform Capability.
**Pełna dokumentacja:** [docs_src/usbd_desc_cw.md](docs_src/usbd_desc_cw.md)

**Nagłówki:** task_net.h, usb_device.h, usb_webusb.h, usbd_conf.h, usbd_conf_cw.h, usbd_desc_cw.h, app_ethernet.h, time_service.h, lwipopts.h

---

## 🎨 User Experience (Interfejs użytkownika)

Zarządzanie wyświetlaczem, obsługa przycisków, zdarzenia systemowe i wskaźniki.

### [task_display.c](Core/Src/task_display.c) — UI SSD1306 (pełna wersja)
Zarządzanie stanem wizualnym na SSD1306 128×32: 4 linie, przewijający się dziennik, łączenie stanu.
**Pełna dokumentacja:** [docs_src/task_display.md](docs_src/task_display.md)

### [task_display_minimal.c](Core/Src/task_display_minimal.c) — UI SSD1306 (minimalny)
Minimalny UI dla `minimal-lwip`: lustrzane odbicie UART, okresowe aktualizacje wyświetlacza.
**Pełna dokumentacja:** [docs_src/task_display_minimal.md](docs_src/task_display_minimal.md)

### [task_user.c](Core/Src/task_user.c) — Przycisk użytkownika (PC13)
Fizyczny UX: tłumienie drgań, krótkie naciśnięcie (Potwierdź) vs długie przytrzymanie ~2,5s (Odrzuć).
**Pełna dokumentacja:** [docs_src/task_user.md](docs_src/task_user.md)

### [task_io.c](Core/Src/task_io.c) — Wskaźniki LED
Wskaźniki wizualne: LED1 = puls aktywności, LED2 = aktywność sieciowa, LED3 = alert bezpieczeństwa.
**Pełna dokumentacja:** [docs_src/task_io.md](docs_src/task_io.md)

**Nagłówki:** task_display.h, task_user.h, task_io.h

---

## ⚙️ System & Hardware (System i Sprzęt)

Inicjalizacja HAL, taktowanie, procedury obsługi przerwań, konfiguracja sterownika.

### [hw_init.c](Core/Src/hw_init.c) — Uruchomienie płyty (zegary, MPU, GPIO, I2C, UART)
Bootstrap niskiego poziomu: konfiguracja zegara, MPU/pamięć podręczna (dla LwIP), GPIO (LED, przycisk), I2C1 (OLED), UART, USB, RNG.
**Pełna dokumentacja:** [docs_src/hw_init.md](docs_src/hw_init.md)

### [stm32h7xx_hal_msp.c](Core/Src/stm32h7xx_hal_msp.c) — Callbacki HAL MSP
Konfiguracja poziomu sprzętowego: `HAL_I2C_MspInit` (I2C1), `HAL_UART_MspInit` (USART3).
**Pełna dokumentacja:** [docs_src/stm32h7xx_hal_msp.md](docs_src/stm32h7xx_hal_msp.md)

### [stm32h7xx_it.c](Core/Src/stm32h7xx_it.c) — Procedury obsługi przerwań (główne)
Procedury obsługi: `SysTick_Handler` (znacznik czasu FreeRTOS), IRQ Ethernet (gdy `USE_LWIP`).
**Pełna dokumentacja:** [docs_src/stm32h7xx_it.md](docs_src/stm32h7xx_it.md)

### [stm32h7xx_it_systick.c](Core/Src/stm32h7xx_it_systick.c) — SysTick dla minimal-lwip
Alternatywny `SysTick_Handler` dla kompilacji `minimal-lwip`.
**Pełna dokumentacja:** [docs_src/stm32h7xx_it_systick.md](docs_src/stm32h7xx_it_systick.md)

### [stm32h7xx_it_usb.c](Core/Src/stm32h7xx_it_usb.c) — IRQ USB OTG HS
Procedura obsługi przerwania OTG HS: wywołuje `HAL_PCD_IRQHandler` dla WebUSB.
**Pełna dokumentacja:** [docs_src/stm32h7xx_it_usb.md](docs_src/stm32h7xx_it_usb.md)

### [ssd1306_conf.h](Drivers/ssd1306/ssd1306_conf.h) — Konfiguracja sterownika wyświetlacza
Parametry czasu kompilacji: powiązanie I2C1, adres 0x3C, geometria 128×32, czcionka 6×8.
**Pełna dokumentacja:** [docs_src/ssd1306_conf.md](docs_src/ssd1306_conf.md)

**Nagłówki:** hw_init.h, main.h, lwipopts.h

---

## 📚 Dokumentacja i odniesienia

- **[docs_src/README.md](docs_src/README.md)** — Kompletny indeks wszystkich 32 modułów z hierarchiczną nawigacją
- **[docs_src/doxygen-comments.md](docs_src/doxygen-comments.md)** — Wytyczne stylu komentarza Doxygen
- **[docs_src/api-documentation-scope.md](docs_src/api-documentation-scope.md)** — Śledzenie postępu dokumentacji

---

## 🚀 Szybki start

### Struktura dokumentacji
1. **Kod (.c/.h)** — minimalny @brief/@details, poziom API
2. **docs_src/*.md** — szczegółowe wyjaśnienia logiki (Abstrakt → Przepływ logiki → Zależności)
3. **Doxygen HTML** — odsyłacze krzyżowe kodu (`make docs-doxygen`)

### Jak przeczytać moduł
1. Otwórz [docs_src/README.md](docs_src/README.md)
2. Znajdź interesujący Cię moduł
3. Zacznij od **Abstraktu** (logika biznesowa) → **Przepływ logiki** (algorytm) → **Zależności**
4. Obserwuj **Relacje** w celu uzyskania szerszego kontekstu

### Główne polecenia
```bash
make docs-doxygen    # Wygeneruj Doxygen
make build          # Zbuduj
make minimal-lwip   # Minimalna kompilacja
make flash          # Wgraj na STM32
```

---

## 📋 Kompletny indeks modułów

<!-- DOXYGEN_DOCS_SRC_INDEX -->
| Moduł | Krótki przegląd |
|--------|------------------|
| [api-documentation-scope](docs_src/api-documentation-scope.md) | Śledzenie postępu dokumentacji i zakresu pokrycia Doxygen dla wszystkich modułów. |
| [app_ethernet](docs_src/app_ethernet.md) | Plik nagłówka definiujący interfejsy warstwy „uszczelki" Ethernet: callback połączenia, stałe FSM DHCP. |
| [app_ethernet_cw](docs_src/app_ethernet_cw.md) | Obsługa Ethernet: FSM up/down, maszyna stanu DHCP (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), sprzężenie zwrotne LED. |
| [crypto_wallet](docs_src/crypto_wallet.md) | Opakowuje bibliotekę trezor-crypto: STM32 RNG z mieszaniem entropii, BIP-39, pochodna HD BIP-32, podpis ECDSA secp256k1. |
| [doxygen-comments](docs_src/doxygen-comments.md) | Wytyczne stylu komentarza Doxygen: separacja @brief/@details w kodzie. |
| [hw_init](docs_src/hw_init.md) | Uruchomienie płyty: konfiguracja zegara, MPU/pamięć podręczna (dla LwIP), GPIO, I2C1, UART, USB, inicjalizacja RNG. |
| [lwipopts](docs_src/lwipopts.md) | Konfiguracja czasu kompilacji LwIP: IPv4/TCP/DHCP/DNS/SNTP, parametry sterty, bufory/okno TCP. |
| [main](docs_src/main.md) | Punkt wejścia FreeRTOS i aranżacja aplikacji. |
| [memzero](docs_src/memzero.md) | Bezpieczne zerowanie buforu przy użyciu zapisów volatile, aby zapobiec optymalizacji kompilatora. |
| [README](docs_src/README.md) | Kompletny indeks 32 modułów z hierarchiczną nawigacją i metodologią dokumentacji. |
| [sha256_minimal](docs_src/sha256_minimal.md) | Kompaktna implementacja SHA-256 dla kompilacji minimalnych (USE_CRYPTO_SIGN=0). |
| [ssd1306_conf](docs_src/ssd1306_conf.md) | Konfiguracja czasu kompilacji sterownika wyświetlacza. |
| [stm32h7xx_hal_msp](docs_src/stm32h7xx_hal_msp.md) | Callbacki HAL MSP dla konfiguracji sprzętu I2C1 i USART3. |
| [stm32h7xx_it](docs_src/stm32h7xx_it.md) | Procedury obsługi przerwań: SysTick (znacznik czasu FreeRTOS), IRQ Ethernet. |
| [stm32h7xx_it_systick](docs_src/stm32h7xx_it_systick.md) | Alternatywna procedura obsługi SysTick dla kompilacji minimal-lwip. |
| [stm32h7xx_it_usb](docs_src/stm32h7xx_it_usb.md) | Procedura obsługi przerwania USB OTG HS dla WebUSB. |
| [task_display](docs_src/task_display.md) | Zarządzanie UI SSD1306: wyświetlacz 4-liniowy, przewijający się dziennik, łączenie stanu. |
| [task_display_minimal](docs_src/task_display_minimal.md) | Minimalny UI dla minimal-lwip: lustrzane odbicie UART, zmniejszone obciążenie SSD1306. |
| [task_io](docs_src/task_io.md) | Zadanie wskaźnika LED: puls aktywności, aktywność sieciowa, alerty bezpieczeństwa. |
| [task_net](docs_src/task_net.md) | Serwer HTTP i API sieciowy: LwIP/Ethernet, port 80, parsowanie i walidacja POST /tx. |
| [task_security](docs_src/task_security.md) | Starsze FSM podpisywania z fikcyjną kryptografią (tylko test/audit). |
| [task_sign](docs_src/task_sign.md) | Główny pipeline podpisywania: konsument kolejki, tworzenie SHA-256, potwierdzenie użytkownika, podpis ECDSA. |
| [task_user](docs_src/task_user.md) | Obsługa przycisku użytkownika: tłumienie drgań, rozróżnienie krótkie/długie naciśnięcie, sygnalizacja potwierdź/odrzuć. |
| [time_service](docs_src/time_service.md) | Synchronizacja czasu SNTP i formatowanie ciągu UTC. |
| [tx_request_validate](docs_src/tx_request_validate.md) | Brama walidacji danych wejściowych: weryfikacja adresu (Base58/bech32), kwoty (decimal), waluty (whitelist). |
| [usb_device](docs_src/usb_device.md) | Inicjalizacja urządzenia USB: zegar HSI48, rdzeń USBD, rejestracja klasy WebUSB. |
| [usb_webusb](docs_src/usb_webusb.md) | Interfejs dostawcy WebUSB: punkty końcowe zbiorcze, ping/pong, żądanie/odpowiedź podpisu binarnego. |
| [usbd_conf](docs_src/usbd_conf.md) | Wrapper konfiguracji urządzenia USB. |
| [usbd_conf_cw](docs_src/usbd_conf_cw.md) | Konfiguracja BSP USB dla CryptoWallet WebUSB. |
| [usbd_desc_cw](docs_src/usbd_desc_cw.md) | Deskryptory urządzenia USB dla WebUSB. |
| [wallet_seed](docs_src/wallet_seed.md) | Zaślepka ziarna testowego (USE_TEST_SEED=1, tylko programowanie). |
| [wallet_shared](docs_src/wallet_shared.md) | Wspólne typy IPC, struktury i kontrakty globalnych obiektów. |
<!-- /DOXYGEN_DOCS_SRC_INDEX -->
