\page usbd_conf_cw "usbd_conf_cw: USB device BSP (PCD MSP + static buffers)"

# `usbd_conf_cw.c` + `usbd_conf_cw.h`

<brief>Module `usbd_conf_cw` is the BSP/configuration layer for USB device middleware: it provides a static allocator for USBD, describes memory hooks, configures MSP for PCD (GPIO alternate function, clock enable, NVIC priority/enable), and bridges HAL_PCD callbacks to USBD_LL events.</brief>

## Overview

<brief>Module `usbd_conf_cw` is the BSP/configuration layer for USB device middleware: it provides a static allocator for USBD, describes memory hooks, configures MSP for PCD (GPIO alternate function, clock enable, NVIC priority/enable), and bridges HAL_PCD callbacks to USBD_LL events.</brief>

## Abstract (Logic Synthesis)

STM32 USB middleware (USBD) requires a binding between:
- Hardware-independent layer (USBD core)
- Board/hardware-dependent layer (MSP init for PCD + translation of HAL events to USBD_LL functions)
- Buffer constraints/allocators

`usbd_conf_cw` provides this "transition" infrastructure. Its business role is to ensure that the WebUSB class in `usb_webusb` can operate atop a correctly configured USB peripheral (PCD) with a predictable set of memory hooks.

## Logic Flow (MSP Init + Callback Bridging)

Main blocks:

### MSP Init/Deinit for PCD

In `HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)`:
1. Check that `USB1_OTG_HS` is used (early exit for others)
2. Enable GPIOA clock
3. Configure PA11/PA12:
   - AF push-pull
   - Pull: nopull
   - Very high speed
   - Alternate function: `GPIO_AF10_OTG1_FS`
4. Enable `USB1_OTG_HS_CLK_ENABLE()`
5. Configure NVIC:
   - Priority (6,0)
   - Enable IRQ `OTG_HS_IRQn`

`HAL_PCD_MspDeInit` does the reverse: disable clock, deinitialize GPIO, disable IRQ.

### Callback Bridging

Functions `HAL_PCD_*Callback` translate HAL-equivalent events to `USBD_LL_*`:
- Setup stage → `USBD_LL_SetupStage`
- DataOut stage → `USBD_LL_DataOutStage`
- DataIn stage → `USBD_LL_DataInStage`
- SOF/Reset/Suspend/Resume, etc.

### USBD Static Memory Hooks

There's `usbd_webusb_mem` array and functions:
- `USBD_static_malloc(size)` returns pointer to static buffer (no real allocation)
- `USBD_static_free(p)` is no-op

## Interrupts/Registers

Register-level operations are not performed directly: NVIC/GPIO configuration is done via HAL.
ISRs are implemented in `stm32h7xx_it_usb.c` and depend on correct IRQ setup in MSP init.

## Timings

Timings are determined by USBD/HAL middleware logic:

| Element | Origin |
|---|---|
| NVIC priority | Fixed constant in MSP init |
| FIFO buffer sizes | Defined in USBD configuration (see PCD init) |

## Dependencies

Direct:
- STM32 HAL: `HAL_PCD_*`, `HAL_GPIO_Init`, RCC/NVIC
- USBD middleware: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_ioreq.h`
- WebUSB class configuration: `usb_webusb.h`

Global:
- `hpcd_USB_FS` declared as external in `usb_device.c` and used in IRQ handler

## Relations

- `stm32h7xx_it_usb.md` — Bridge from IRQ to HAL_PCD_IRQHandler
- `usb_device.md` — Middleware init startup
- `usb_webusb.md` — Class/EP/commands
