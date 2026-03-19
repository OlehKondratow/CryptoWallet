\page usbd_conf "usbd_conf: Przekierowanie konfiguracji USB device"

# `Core/Inc/usbd_conf.h`

<brief>Nagłówek `usbd_conf` jest cienką opaką: zawiera `usbd_conf_cw.h`, tym samym "wiążąc" konfigurację oprogramowania pośredniego USB device do konkretnej implementacji WebUSB BSP CryptoWallet.</brief>

## Przegląd

<brief>Nagłówek `usbd_conf` jest cienką opaką: zawiera `usbd_conf_cw.h`, tym samym "wiążąc" konfigurację oprogramowania pośredniego USB device do konkretnej implementacji WebUSB BSP CryptoWallet.</brief>

## Abstrakcja (synteza logiki)

Oprogramowanie pośrednie USBD oczekuje, że projekt będzie dostarczać nagłówek konfiguracyjny (`usbd_conf.h`). Aby uniknąć duplikacji i utrzymać konfigurację na "poziomie projektu", ten plik przekierowuje do `usbd_conf_cw.h`, który zawiera rzeczywistą logikę BSP (inicjalizacja MSP, bufory statyczne, hooki malloc).

## Przepływ logiki

To jest przekierowanie czasu kompilacji:
1. Dołącz `usbd_conf_cw.h`
2. Wyeksportuj jego definicje na zewnątrz

## Przerwania/rejestry

Żaden.

## Czasy

Brak.

## Zależności

- `usbd_conf_cw.h`

## Relacje

- `usbd_conf_cw.md` — Rzeczywista konfiguracja BSP
- `usb_device.md` / `usb_webusb.md` — Użyj oprogramowania pośredniego USBD i otrzymuj konfigurację poprzez łańcuch include
