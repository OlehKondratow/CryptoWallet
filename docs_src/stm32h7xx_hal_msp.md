\page stm32h7xx_hal_msp "stm32h7xx_hal_msp: HAL MspInit for I2C1 + UART3"

# `stm32h7xx_hal_msp.c`

<brief>File `stm32h7xx_hal_msp` sets up hardware layer for HAL: `HAL_I2C_MspInit` configures I2C1 (PB8/PB9 as AF OD with pull-up), and `HAL_UART_MspInit` configures USART3 (TX/RX pins and clocks). This ensures `hw_init` can initialize I2C peripherals and UART without manual pin configuration in the application.</brief>

## Overview

<brief>File `stm32h7xx_hal_msp` sets up hardware layer for HAL: `HAL_I2C_MspInit` configures I2C1 (PB8/PB9 as AF OD with pull-up), and `HAL_UART_MspInit` configures USART3 (TX/RX pins and clocks). This ensures `hw_init` can initialize I2C peripherals and UART without manual pin configuration in the application.</brief>

## Abstract (Logic Synthesis)

HAL in STM32 divides two tasks:
1) "What" to initialize (peripheral parameters) — done in `hw_init` and HAL init calls
2) "How" to connect pins/clocks/IRQs — done in MSP layer (`stm32h7xx_hal_msp.c`)

This file implements the second layer, linking pin macros from `main.h` to corresponding HAL structures.

## Logic Flow (MSP Init)

### I2C MSP

`HAL_I2C_MspInit(hi2c)`:
1. If `hi2c->Instance == I2Cx`:
   - Enable GPIO port clock
   - Enable I2C clock
   - Configure SCL/SDA:
     - Mode: `GPIO_MODE_AF_OD`
     - Pull-up: `GPIO_PULLUP`
     - Speed: Low
     - Alternate function: `I2Cx_AF`

`HAL_I2C_MspDeInit` uses force-reset approach: reset/release and pin deinitialization.

### UART MSP

`HAL_UART_MspInit(huart)`:
1. If `huart->Instance == USARTx`:
   - Configure peripheral clock selection for USART234578
   - Enable clocks for TX/RX GPIO and USART
   - Configure TX/RX pins:
     - Alternate functions: `USARTx_TX_AF` and `USARTx_RX_AF`
     - Mode: AF_PP, pull-up, very high speed

`HAL_UART_MspDeInit` performs force-reset/release and pin deinitialization.

## Interrupts/Registers

File does not implement ISR. It works through HAL GPIO/RCC interfaces (register operations are inside HAL).

## Timings

None. This is static configuration before peripheral startup.

## Dependencies

Direct:
- Macros from `main.h`: `I2Cx_*`, `USARTx_*`, clock enable macros
- HAL types: `I2C_HandleTypeDef`, `UART_HandleTypeDef`

Downstream impact:
- `hw_init.md` / `hw_init.c` calls `HAL_I2C_Init` and `HAL_UART_Init`; MSP provides correct pinmux/clock

## Relations

- `hw_init.md` — Caller of HAL init functions
- `ssd1306_conf.md` — I2C target for SSD1306 display
