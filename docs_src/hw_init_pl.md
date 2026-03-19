\page hw_init "hw_init: board bring-up (clock/MPU/GPIO/I2C/UART/USB)"
\related HW_Init
\related HW_Init_Early_LwIP
\related UART_Log

# `hw_init.c` + `hw_init.h`

<brief>Moduł `hw_init` obsługuje inicjalizację sprzętu niskiego poziomu: ustanawia prawidłową kolejność zegara/pamięci podręcznej/MPU (krytyczne dla LwIP/ETH), inicjalizuje GPIO dla UX (LED/przycisk), konfiguruje I2C1 dla SSD1306 i logowania UART, plus opcjonalnie USB i RNG.</brief>

## Przegląd
<brief>Moduł `hw_init` obsługuje inicjalizację sprzętu niskiego poziomu: ustanawia prawidłową kolejność zegara/pamięci podręcznej/MPU (krytyczne dla LwIP/ETH), inicjalizuje GPIO dla UX (LED/przycisk), konfiguruje I2C1 dla SSD1306 i logowania UART, plus opcjonalnie USB i RNG.</brief>

## Abstract (Synteza logiki)
`hw_init` jest "warstwą zaufania" między HAL/Cube a aplikacją: gwarantuje, że przed jakąkolwiek logiką sieciową (gdy `USE_LWIP` jest włączony), MPU i pamięć podręczna są prawidłowo skonfigurowane dla DMA i regionów pamięci; następnie gwarantuje, że urządzenia peryferyjne potrzebne zadaniom (I2C/OLED, UART i podstawowe GPIO) są dostępne w oczekiwanym stanie.

## Logic Flow (sekwencja bootstrap)
Istnieją dwa krytyczne punkty wejścia:
1. `HW_Init_Early_LwIP()` uruchamia się przed `HAL_Init()` i jest aktywny tylko gdy `USE_LWIP` jest ustawiony.
2. `HW_Init()` uruchamia się po `HAL_Init()` i obsługuje główny bring-up: zegary, GPIO, I2C1, UART3 i opcjonalnie USB/RNG; inicjalizacja OLED zależy od `SKIP_OLED`.

## Analiza szczegółowa

### Cel i granice odpowiedzialności
`hw_init` opakowuje inicjalizację HAL. Ze względu na projekt:
- `main.c` przygotowuje IPC/zadania i uruchamia FreeRTOS, wywołując funkcje inicjalizacji w prawidłowej kolejności.
- `hw_init.c` utrzymuje porządek "sprzętu": zegary, pamięć podręczna/MPU, urządzenia peryferyjne (GPIO/I2C/UART) i zależności od flag kompilacji.
- Logika sieciowa (Ethernet PHY/DHCP/link) nie żyje tutaj bezpośrednio: jest włączana przez środowisko LwIP/Cube i wdrażana w innych modułach (np. poprzez Cube/BSP + `Src/app_ethernet_cw.c`).

### Punkt wejścia #1: `HW_Init_Early_LwIP()` (tylko gdy `USE_LWIP`)
Ta funkcja jest wywoływana z `main.c` **przed** `HAL_Init()` aby upewnić się, że Cortex-M7 prawidłowo obsługuje DMA/regiony pamięci używane przez Ethernet/LwIP.

Kluczowa logika wewnątrz:
1. `HAL_MPU_Disable()` — bezpiecznie rekonfiguruj regiony.
2. Skonfiguruj trzy regiony MPU:
   - **Region 0**: "odmów wszystko" (4GB bez dostępu) jako domyślne.
   - **Region 1**: adres **`0x30000000`** (1 KB) dla **deskryptorów ETH** (deskryptory DMA).
   - **Region 2**: adres **`0x30004000`** (16 KB) dla **sterty LwIP**.
3. `HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT)` — włącz MPU.
4. Włącz pamięci podręczne: `SCB_EnableICache()` i `SCB_EnableDCache()`.

Dlaczego to jest ważne dla DMA:
- Ethernet DMA czyta/pisze pamięć; jeśli adresy deskryptorów/sterty nie są prawidłowo skonfigurowane, pamięć podręczna może przechwycić dane, powodując desynchronizację.
- Konkretne adresy bazowe/rozmiary są dostrajane do potoku LwIP/Cube (szablon `lwip_zero`).

### Punkt wejścia #2: `HW_Init()` (po `HAL_Init()`)
`HW_Init()` — główny bring-up po `HAL_Init()`:

1. **Kompilacja bez LwIP (`!USE_LWIP`)**
   - Wywołuje `MPU_Config()` (aktualnie `HAL_MPU_Disable()`; unika utrzymywania MPU aktywnego niepotrzebnie).
   - `CPU_CACHE_Enable()` uruchamia się później, po przełączeniu zegara (rekomendowane dla H7).

2. **Opóźnienie po włączeniu** (pusta pętla ~2M iteracji)
   - Komentarz wskazuje stabilizację zegara MCO/ST-Link na Nucleo.

3. **`SystemClock_Config()`**
   - Włącza zasilanie (regulator PWR scale1).
   - Włącza zegar **D2 SRAM3**: `__HAL_RCC_D2SRAM3_CLK_ENABLE()`.
   - Konfiguruje PLL z HSE, ustawia dzielniki AHB/APB. Gwarantuje prawidłowe częstotliwości dla HAL i urządzeń peryferyjnych.

4. `SystemCoreClockUpdate()`
   - Synchronizuje zmienne HAL z rzeczywistym zegarem po przełączeniu SYSCLK.

5. Inicjalizacja urządzenia peryferyjnego:
   - `MX_GPIO_Init()`:
     - Włącza zegary dla portów LED1/LED2/LED3 i przycisku `USER`.
     - Konfiguruje diody LED jako `GPIO_MODE_OUTPUT_PP`, niska prędkość, poziomy "wyłączone".
     - Konfiguruje przycisk jako `GPIO_MODE_INPUT`.
   - `MX_I2C1_Init()`:
     - Konfiguruje `hi2c1` (Timing dopasowanie odniesienia, adresowanie 7-bitowe).
     - Włącza filtr analogowy, wyłącza cyfrowy.
     - Wywołuje `ssd1306_Init()` bezpośrednio po `MX_I2C1_Init()`, tylko gdy `USE_LWIP && !SKIP_OLED`.
   - `MX_USART3_Init()`:
     - `USARTx` na 115200, 8N1, tryb TX/RX.
     - Brak hardware flow control, oversampling 16.

6. Opcjonalne gałęzie:
   - `USE_WEBUSB`:
     - Logowanie via UART poprzez `Task_Display_Log()`.
     - `MX_USB_Device_Init()`.
     - Drugie logowanie "USB ready".
   - `USE_CRYPTO_SIGN`:
     - `MX_RNG_Init()` — hardware RNG via HAL (`hrng` + `HAL_RNG_Init()`).
     - Również udostępnia `HAL_RNG_MspInit()` dla włączenia zegara `__HAL_RCC_RNG_CLK_ENABLE()`.

### Interakcja z `main.c` i innymi modułami
W `Core/Src/main.c`, kolejność jest:
1. `HW_Init_Early_LwIP()` (tylko gdy `USE_LWIP`)
2. `HAL_Init()`
3. `HW_Init()`
Następnie tworzy się kolejki/semafory i uruchamia zadania (`Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`).

Praktyczne uzasadnienie dla tego porządku:
- Gałąź LwIP wymaga MPU/cache przed HAL, aby Ethernet DMA i stos LwIP działały prawidłowo.
- Gałąź "OLED init" jest powiązana z I2C: `ssd1306_Init()` uruchamia się bezpośrednio po `MX_I2C1_Init()`.
- UART i USB/RNG (zgodnie z flagami) są przygotowywane w `HW_Init()` aby wczesne logowanie i podstawa kryptograficzna były dostępne przed główną logiką zadań.

### Poziom rejestru/parametru bez przepisywania Cube
`hw_init` nie zastępuje Cube; dokonuje ukierunkowanych dostosowań HAL:
- Zegar/PLL i dzielniki systemowe via `RCC_*` i `HAL_RCC_*`.
- MPU via `HAL_MPU_ConfigRegion`.
- Urządzenia peryferyjne via HAL `HAL_GPIO_Init`, `HAL_I2C_Init`, `HAL_UART_Init`.
Szczegóły pinmux i inicjalizacji MSP dla I2C/USART zazwyczaj znajdują się w `stm32h7xx_hal_msp.c` (np. I2C1 i USART3).

## Przerwania/rejestry
`hw_init` jest unikalnym miejscem, w którym sterowniki systemu (MPU/cache/potok zegara) są wyraźnie modyfikowane poprzez HAL/CMSIS:
- **MPU:** Regiony (w gałęzi LwIP) ustawiają dostęp/atrybuty dla deskryptorów DMA ETH i sterty LwIP.
- **Cache:** I/D-cache włączony po konfiguracji MPU.
- **RCC/PWR:** Wybór PLL, włączenie zegara D2 SRAM3, konfiguracja dzielnika.

## Relacje
- Używa: `main.c` (sekwencja wywołań), `hw_init.h` (API), HAL (`stm32h7xx_hal.h` i sub-moduły), FreeRTOS pośrednio (via `Task_Display_Log`, ale sama inicjalizacja RTOS nie dzieje się tutaj).
- I2C/OLED: `ssd1306.h`, `ssd1306_fonts.h`, deskryptor `hi2c1`.
- Logowanie UART: `UART_Log()` (eksportowany w interfejsie `hw_init.h`), deskryptor `huart3`.
- USB: `usb_device.h` i `MX_USB_Device_Init()` gdy `USE_WEBUSB`.
- RNG/crypto: `RNG_HandleTypeDef` + `HAL_RNG_Init()` gdy `USE_CRYPTO_SIGN`.
- Flagi kompilacji: `USE_LWIP`, `SKIP_OLED`, `USE_WEBUSB`, `USE_CRYPTO_SIGN`.
