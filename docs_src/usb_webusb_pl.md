\page usb_webusb "usb_webusb: interfejs WebUSB vendor (ping + sign)"
\related WebUSB_NotifySignatureReady
\related usb_webusb

# `usb_webusb.c` + `usb_webusb.h`

<brief>Moduł `usb_webusb` implementuje interfejs WebUSB specyficzny dla vendora: otwiera bulk endpoints, obsługuje polecenie `ping` (→ `pong`) i odbiera binarnie oprawioną prośbę o podpis, wyodrębnia `recipient/amount/currency`, waliduje je i kolejkuje transakcję do `g_tx_queue`; gdy podpis jest gotowy, wysyła 64-bajtowy kompaktowy `r||s` poprzez `WebUSB_NotifySignatureReady()`.</brief>

## Przegląd

Moduł `usb_webusb` implementuje interfejs WebUSB specyficzny dla vendora: otwiera bulk endpoints, obsługuje polecenie `ping` i odbiera binarnie oprawioną prośbę o podpis, wyodrębnia recipient/amount/currency, waliduje je i kolejkuje transakcję.

## Przepływ logiki (USB class state + command handling)

### Przygotowanie klasy

Włączone tylko gdy `USE_WEBUSB=1`:
1. `USBD_WEBUSB_Init()`:
   - przechowaj `s_pdev`
   - otwórz EP IN (0x81) i EP OUT (0x02) jako bulk z MPS = 64 bajty
   - prześlij OUT transfery dla odbioru poleceń

2. `USBD_WEBUSB_DataOut()` (callback OUT endpoint):
   - odbierz bufor polecenia (ping lub sign_request frame)
   - sparsuj i przetwórz odpowiednio

### Przetwarzanie poleceń

| Polecenie | Input | Output | Działanie |
|-----------|-------|--------|-----------|
| `ping` | 4-bajt "PING" | "PONG" | Echo health check |
| `sign_request` | recipient\|amount\|currency bytes | queued to `g_tx_queue` | Validate + enqueue |

## Zależności

Bezpośrednie:
- USBD core i framework sterownika klasy
- Zarządzanie USB endpoint (HAL USB)
- `tx_request_validate()` do walidacji
- `g_tx_queue` do kolejowania transakcji
- `g_last_sig[]` i `g_last_sig_ready` do odczytu podpisu

Pośrednie:
- `task_sign.md` (generowanie podpisu)
- `usb_device.md` (rejestracja klasy)

## Relacje modułów

- `usb_device.md` (host sterownika klasy)
- `task_sign.md` (dostawca podpisu)
- `tx_request_validate.md` (walidacja wejścia)
- `wallet_shared.md` (kontrakt transakcji)
