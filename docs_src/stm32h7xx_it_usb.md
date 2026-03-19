\page stm32h7xx_it_usb "stm32h7xx_it_usb: OTG_HS IRQ → HAL_PCD_IRQHandler"

# `stm32h7xx_it_usb.c`

<brief>File `stm32h7xx_it_usb` implements the OTG HS interrupt handler for WebUSB mode: the ISR calls `HAL_PCD_IRQHandler` for `hpcd_USB_FS` so USB device middleware can properly service endpoint transfers.</brief>

## Overview

<brief>File `stm32h7xx_it_usb` implements the OTG HS interrupt handler for WebUSB mode: the ISR calls `HAL_PCD_IRQHandler` for `hpcd_USB_FS` so USB device middleware can properly service endpoint transfers.</brief>

## Abstract (Logic Synthesis)

USB transfers and link/endpoint change events arrive via IRQ. In this project, the WebUSB class sits atop STM32 USB device middleware, and the middleware expects a specific IRQ to be routed to HAL/PCD handlers. This file provides exactly that routing: OTG_HS IRQ → HAL handler.

## Logic Flow (ISR Routing)

1. Under conditional compilation (`USE_WEBUSB==1`), external handle `hpcd_USB_FS` is declared
2. `OTG_HS_IRQHandler()`:
   - Call `HAL_PCD_IRQHandler(&hpcd_USB_FS)`

## Interrupts/Registers

This is an ISR. Registers are not directly manipulated; HAL takes responsibility for clearing IRQ flags and advancing the USB middleware state machine.

## Timings

ISR should be short: only a single HAL call here.

## Dependencies

- STM32 HAL USB device: `HAL_PCD_IRQHandler`
- Handler is expected to be configured in `usbd_conf_cw.c` (NVIC enable/priority)

## Relations

- `usbd_conf_cw.md` — Configures IRQ
- `usb_webusb.md` — Receives DataOut/DataIn via middleware
