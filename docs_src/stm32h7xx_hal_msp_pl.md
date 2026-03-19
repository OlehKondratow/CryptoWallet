\page stm32h7xx_hal_msp "stm32h7xx_hal_msp: HAL MspInit dla I2C1 + UART3"

# `stm32h7xx_hal_msp.c`

<brief>Plik `stm32h7xx_hal_msp` konfiguruje warstwę sprzętu dla HAL: `HAL_I2C_MspInit` konfiguruje I2C1 (PB8/PB9 jako AF OD z pull-up), a `HAL_UART_MspInit` konfiguruje USART3 (piny TX/RX i zegary). Zapewnia to, że `hw_init` może inicjalizować peryferia I2C i UART bez ręcznej konfiguracji pinów w aplikacji.</brief>

## Przegląd

<brief>Plik `stm32h7xx_hal_msp` konfiguruje warstwę sprzętu dla HAL: `HAL_I2C_MspInit` konfiguruje I2C1 (PB8/PB9 jako AF OD z pull-up), a `HAL_UART_MspInit` konfiguruje USART3 (piny TX/RX i zegary). Zapewnia to, że `hw_init` może inicjalizować peryferia I2C i UART bez ręcznej konfiguracji pinów w aplikacji.</brief>

## Abstrakcja (synteza logiki)

HAL w STM32 dzieli dwa zadania:
1) "Co" inicjalizować (parametry peryferii) — wykonywane w `hw_init` i wywołaniach HAL init
2) "Jak" podłączyć piny/zegary/IRQ — wykonywane w warstwie MSP (`stm32h7xx_hal_msp.c`)

Ten plik implementuje drugą warstwę, łącząc makra pinów z `main.h` z odpowiadającymi strukturami HAL.

## Przepływ logiki (inicjalizacja MSP)

### I2C MSP

`HAL_I2C_MspInit(hi2c)`:
1. Jeśli `hi2c->Instance == I2Cx`:
   - Włącz zegar portu GPIO
   - Włącz zegar I2C
   - Skonfiguruj SCL/SDA:
     - Tryb: `GPIO_MODE_AF_OD`
     - Pull-up: `GPIO_PULLUP`
     - Prędkość: Niska
     - Funkcja alternatywna: `I2Cx_AF`

`HAL_I2C_MspDeInit` używa podejścia force-reset: reset/release i deinicjalizacja pinów.

### UART MSP

`HAL_UART_MspInit(huart)`:
1. Jeśli `huart->Instance == USARTx`:
   - Skonfiguruj wybór zegara peryferii dla USART234578
   - Włącz zegary dla TX/RX GPIO i USART
   - Skonfiguruj piny TX/RX:
     - Funkcje alternatywne: `USARTx_TX_AF` i `USARTx_RX_AF`
     - Tryb: AF_PP, pull-up, bardzo wysoka prędkość

`HAL_UART_MspDeInit` wykonuje force-reset/release i deinicjalizację pinów.

## Przerwania/rejestry

Plik nie implementuje ISR. Pracuje poprzez interfejsy HAL GPIO/RCC (operacje rejestru wewnątrz HAL).

## Czasy

Brak. To jest statyczna konfiguracja przed uruchomieniem peryferii.

## Zależności

Bezpośrednie:
- Makra z `main.h`: `I2Cx_*`, `USARTx_*`, makra włączenia zegara
- Typy HAL: `I2C_HandleTypeDef`, `UART_HandleTypeDef`

Wpływ w dół:
- `hw_init.md` / `hw_init.c` wywołuje `HAL_I2C_Init` i `HAL_UART_Init`; MSP zapewnia prawidłowy pinmux/clock

## Relacje

- `hw_init.md` — Wywołujący HAL init funkcje
- `ssd1306_conf.md` — Cel I2C dla wyświetlacza SSD1306
