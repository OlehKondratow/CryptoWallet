\page usbd_conf "usbd_conf: USB device config redirect"

# `Core/Inc/usbd_conf.h`

<brief>Header `usbd_conf` is a thin wrapper: it includes `usbd_conf_cw.h`, thereby "binding" USB device middleware configuration to CryptoWallet's specific WebUSB BSP implementation.</brief>

## Overview

<brief>Header `usbd_conf` is a thin wrapper: it includes `usbd_conf_cw.h`, thereby "binding" USB device middleware configuration to CryptoWallet's specific WebUSB BSP implementation.</brief>

## Abstract (Logic Synthesis)

USBD middleware expects the project to provide a configuration header (`usbd_conf.h`). To avoid duplication and keep configuration at the "project level", this file redirects to `usbd_conf_cw.h`, which contains the actual BSP logic (MSP init, static buffers, malloc hooks).

## Logic Flow

This is compile-time redirection:
1. Include `usbd_conf_cw.h`
2. Export its definitions outward

## Interrupts/Registers

None.

## Timings

None.

## Dependencies

- `usbd_conf_cw.h`

## Relations

- `usbd_conf_cw.md` — Actual BSP configuration
- `usb_device.md` / `usb_webusb.md` — Use USBD middleware and receive configuration via include chain
