\page usbd_desc_cw "usbd_desc_cw: Deskryptory WebUSB USB + BOS"

# `usbd_desc_cw.c` + `usbd_desc_cw.h`

<brief>Moduł `usbd_desc_cw` konstruuje deskryptory USB dla urządzenia WebUSB: device/interface/BOS i ciągi (producent/produkt/serial), w tym UUID Platform Capability WebUSB, a także dynamiczna generacja numeru seryjnego na podstawie rejestrów STM32 DEVICE_ID.</brief>

## Przegląd

<brief>Moduł `usbd_desc_cw` konstruuje deskryptory USB dla urządzenia WebUSB: device/interface/BOS i ciągi (producent/produkt/serial), w tym UUID Platform Capability WebUSB, a także dynamiczna generacja numeru seryjnego na podstawie rejestrów STM32 DEVICE_ID.</brief>

## Abstrakcja (synteza logiki)

Host USB dla WebUSB musi otrzymać prawidłowy zestaw deskryptorów urządzenia, w tym informacje, dzięki którym Chrome/API hosta rozpoznają to jako urządzenie WebUSB. `usbd_desc_cw` rozwiązuje to zadanie: opisuje VID/PID, konstruuje ciągi, udostępnia capability BOS/WebUSB i generuje numer seryjny, aby host mógł odróżnić urządzenia.

## Przepływ logiki (dostawca deskryptorów)

Oprogramowanie pośrednie USBD wywołuje poprzez wskaźniki funkcji w obiekcie `WEBUSB_Desc`:
1. Gdy żąda deskryptora device/config/interface/string/BOS:
   - Zwróć wstępnie przygotowane tablice (device/lang/bos itp.)
   - Ciągi producent/produkt są generowane poprzez `USBD_GetString(...)` w locie
   - Ciąg seryjny jest generowany poprzez `Get_SerialNum()`
2. `Get_SerialNum()`:
   - Odczytaj `DEVICE_ID1/2/3`
   - Połącz je (d0 += d2)
   - Konwertuj liczby na znaki heksadecymalne poprzez `IntToUnicode()`

Niezmienniki:

| Parametr | Wartość |
|---|---|
| VID/PID | `0x1209 / 0xC0DE` |
| Prędkość USB | `USBD_SPEED_FULL` (prędkość używana w init) |
| UUID WebUSB (capability BOS) | `3408b638-09a9-47a0-8bfd-a0768815b665` (w tablicy deskryptora) |

## Przerwania/rejestry

Brak ISR. Jest odczyt rejestrów ID urządzenia (wartości memory-mapped poprzez makra DEVICE_ID*).

## Czasy

Brak konkretnych ograniczeń czasowych: deskryptory są zwracane podczas połączenia/żądań hosta.

## Zależności

Bezpośrednie:
- Typy deskryptorów oprogramowania pośredniego USBD: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_def.h`
- Konfiguracja: `usbd_conf_cw.h` (dla size/macros)
- Zewnętrzny `WEBUSB_Desc` używany przez `usb_device.c` w `USBD_Init`

## Relacje

- `usb_device.md` — Wywołuje `USBD_Init(... &WEBUSB_Desc ...)`
- `usb_webusb.md` — Zawiera deskryptor klasy/config (EP layout)
