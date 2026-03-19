\page usb_device "usb_device: WebUSB device init (clock + USBD core)"
\related MX_USB_Device_Init

# `usb_device.c` + `usb_device.h`

<brief>The `usb_device` module brings up USB device for WebUSB: it configures USB clock source (HSI48), then starts USBD core, registers WebUSB class, and starts USB event handling.</brief>

## Overview

The `usb_device` module brings up USB device for WebUSB: it configures USB clock source (HSI48), then starts USBD core, registers WebUSB class, and starts USB event handling.

## Logic Flow (Init Sequence)

Algorithm when `USE_WEBUSB=1`:
1. `MX_USB_Device_Init()`:
   - call `USBD_Clock_Config()`
   - `USBD_Init(&hUsbDeviceFS, &WEBUSB_Desc, USBD_SPEED_FULL)`
   - `USBD_RegisterClass(&hUsbDeviceFS, &USBD_WEBUSB_ClassDriver)`
   - `USBD_Start(&hUsbDeviceFS)`
2. `USBD_Clock_Config()`:
   - select `RCC_PERIPHCLK_USB` and `RCC_USBCLKSOURCE_HSI48`
   - enable HSI48 oscillator
   - check HAL return statuses, call `Error_Handler()` on errors

## Interrupts and Registers

Module contains no ISR: all IRQ handling delegated to USB middleware and handlers in `stm32h7xx_it_usb.c`.
No direct register work (only via HAL RCC configuration).

## Dependencies

Direct:
- HAL RCC clock configuration
- USBD core from STM32 USB middleware
- WebUSB class driver implementation
- USB interrupt handlers (in `stm32h7xx_it_usb.c`)

Indirect:
- `usb_webusb.md` (class driver)
- `task_net.md` (application may use USB as alternative to Ethernet)

## Module Relationships

- `usb_webusb.md` (vendor class implementation)
- `stm32h7xx_it_usb.md` (interrupt handlers)
- `hw_init.md` (related clock setup)
