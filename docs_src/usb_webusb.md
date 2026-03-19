\page usb_webusb "usb_webusb: WebUSB vendor interface (ping + sign)"
\related WebUSB_NotifySignatureReady
\related usb_webusb

# `usb_webusb.c` + `usb_webusb.h`

<brief>The `usb_webusb` module implements vendor-specific WebUSB interface: it opens bulk endpoints, supports `ping` command (→ `pong`) and receives binary-framed signature request, extracts `recipient/amount/currency`, validates them and queues transaction to `g_tx_queue`; when signature ready, sends 64-byte compact `r||s` via `WebUSB_NotifySignatureReady()`.</brief>

## Overview

The `usb_webusb` module implements vendor-specific WebUSB interface: it opens bulk endpoints, supports `ping` command and receives binary-framed signature request, extracts recipient/amount/currency, validates them and queues transaction; when signature ready, sends 64-byte compact r||s signature.

## Logic Flow (USB Class State + Command Handling)

### Class Preparation

Enabled only when `USE_WEBUSB=1`:
1. `USBD_WEBUSB_Init()`:
   - store `s_pdev`
   - open EP IN (0x81) and EP OUT (0x02) as bulk with MPS = 64 bytes
   - post OUT transfers for command reception

2. `USBD_WEBUSB_DataOut()` (OUT endpoint callback):
   - receive command buffer (ping or sign_request frame)
   - parse and process accordingly

### Command Processing

| Command | Input | Output | Action |
|---------|-------|--------|--------|
| `ping` | 4-byte "PING" | "PONG" | Echo health check |
| `sign_request` | recipient|amount|currency| bytes | queued to `g_tx_queue` | Validate + enqueue |

## Dependencies

Direct:
- USBD core and class driver framework
- USB endpoint management (HAL USB)
- `tx_request_validate()` for validation
- `g_tx_queue` for transaction queueing
- `g_last_sig[]` and `g_last_sig_ready` for signature readout

Indirect:
- `task_sign.md` (signature generation)
- `usb_device.md` (class registration)

## Module Relationships

- `usb_device.md` (class driver host)
- `task_sign.md` (signature provider)
- `tx_request_validate.md` (input validation)
- `wallet_shared.md` (transaction contract)
