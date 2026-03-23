# 2. Struktura oprogramowania układowego

## 2.1 MCU i układ pamięci

- **MCU:** STM32H743ZI (Cortex-M7), cel NUCLEO-H743ZI2.
- **Flash:** aplikacja linkowana dla buildu z LwIP w `STM32H743ZITx_FLASH_LWIP.ld` (patrz `LDSCRIPT` w Makefile).
- **OS:** FreeRTOS (miejscami API CMSIS-RTOS2).

## 2.2 Inicjalizacja

- **`Core/Src/main.c`** — zegary, start RTOS, tworzenie zadań i prymitywów IPC.
- **`Core/Src/hw_init.c`** — zegary, zasady MPU/cache pod DMA Ethernet, GPIO, I2C (OLED), UART, USB, RNG zgodnie z flagami kompilacji.

## 2.3 Zadania (pojęciowo)

| Obszar | Typowe pliki | Rola |
|--------|--------------|------|
| Wyświetlacz | `task_display.c`, `task_display_minimal.c` | OLED, podsumowanie transakcji |
| Sieć | `Src/task_net.c` | Serwer HTTP, parsowanie żądań, kolejki do podpisu |
| Podpis | `task_sign.c` | Walidacja żądania, UI, ECDSA, odpowiedź |
| Użytkownik / IO | `task_user.c`, `task_io.c` | Debounce przycisku, LED |
| USB | `usb_device.c`, `usb_webusb.c`, `usbd_*_cw.c` | WebUSB przy `USE_WEBUSB=1` |
| UART | `cwup_uart.c`, `rng_dump.c` | CWUP vs surowy strumień RNG (wykluczają się na tym samym UART) |
| Integralność | `fw_integrity.c` | Start + opcjonalnie `AT+FWINFO?` |

Ścieżki legacy lub audytowe mogą obejmować `task_security.c` — sprawdź źródła przed użyciem w produkcie.

## 2.4 IPC i wspólne typy

- **`Core/Inc/wallet_shared.h`** — typy transakcji, uchwyty kolejek, kontekst wyświetlacza, limity (`TX_*_LEN`).
- **Kolejki** — np. sieć → podpis → wyświetlacz; rozmiary i polityka odrzuceń to część poprawności pod obciążeniem (patrz źródła).

## 2.5 Mapa modułów (źródło prawdy w kodzie)

| Obszar | Główne pliki |
|--------|--------------|
| Trasy HTTP | `Src/task_net.c` |
| Walidacja żądań | `Core/Src/tx_request_validate.c` |
| Wrapper krypto | `Core/Src/crypto_wallet.c` |
| Bezpieczne zerowanie | `Core/Src/memzero.c` |
| Ethernet / DHCP | `Src/app_ethernet_cw.c` |
| Czas | `Core/Src/time_service.c` |
| Błędy | `Core/Src/fault_report.c`, `stm32h7xx_it*.c` |

## 2.6 Komponenty zewnętrzne

- **`ThirdParty/trezor-crypto`** — BIP-39/32, secp256k1, skróty (przy `USE_CRYPTO_SIGN=1`).
- **STM32 HAL / CMSIS** — przez drzewo `STM32CubeH7` (`CRYPTO_DEPS_ROOT` lub sąsiedni checkout).
