\page usbd_desc_cw "usbd_desc_cw: WebUSB USB descriptors + BOS"

# `usbd_desc_cw.c` + `usbd_desc_cw.h`

<brief>Module `usbd_desc_cw` constructs USB descriptors for WebUSB device: device/interface/BOS and strings (manufacturer/product/serial), including WebUSB Platform Capability UUID, plus dynamic serial number generation based on STM32 DEVICE_ID registers.</brief>

## Overview

<brief>Module `usbd_desc_cw` constructs USB descriptors for WebUSB device: device/interface/BOS and strings (manufacturer/product/serial), including WebUSB Platform Capability UUID, plus dynamic serial number generation based on STM32 DEVICE_ID registers.</brief>

## Abstract (Logic Synthesis)

The USB host for WebUSB must receive a correct set of device descriptors, including information by which Chrome/host APIs recognize this as a WebUSB device. `usbd_desc_cw` solves this task: it describes VID/PID, constructs strings, provides BOS/WebUSB capability, and generates a serial number so the host can distinguish devices.

## Logic Flow (Descriptor Provider)

USBD middleware calls through function pointers in the `WEBUSB_Desc` object:
1. When requesting device/config/interface/string/BOS descriptor:
   - Return pre-prepared arrays (device/lang/bos, etc.)
   - Manufacturer/product strings are generated via `USBD_GetString(...)` on-the-fly
   - Serial string is generated via `Get_SerialNum()`
2. `Get_SerialNum()`:
   - Read `DEVICE_ID1/2/3`
   - Combine them (d0 += d2)
   - Convert numbers to hex characters via `IntToUnicode()`

Invariants:

| Parameter | Value |
|---|---|
| VID/PID | `0x1209 / 0xC0DE` |
| USB speed | `USBD_SPEED_FULL` (speed used in init) |
| WebUSB UUID (BOS capability) | `3408b638-09a9-47a0-8bfd-a0768815b665` (in descriptor array) |

## Interrupts/Registers

No ISR. There is reading of device ID registers (memory-mapped values via DEVICE_ID* macros).

## Timings

No specific timing constraints: descriptors are returned during connect/host requests.

## Dependencies

Direct:
- USBD middleware descriptor types: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_def.h`
- Configuration: `usbd_conf_cw.h` (for size/macros)
- External `WEBUSB_Desc` used by `usb_device.c` in `USBD_Init`

## Relations

- `usb_device.md` — Calls `USBD_Init(... &WEBUSB_Desc ...)`
- `usb_webusb.md` — Contains class/config descriptor (EP layout)
