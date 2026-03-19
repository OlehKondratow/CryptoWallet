\page usb_device "usb_device: inicjalizacja urządzenia WebUSB (zegar + USBD core)"
\related MX_USB_Device_Init

# `usb_device.c` + `usb_device.h`

<brief>Moduł `usb_device` uruchamia urządzenie USB dla WebUSB: konfiguruje źródło zegara USB (HSI48), następnie uruchamia USBD core, rejestruje klasę WebUSB i uruchamia obsługę zdarzeń USB.</brief>

## Przegląd

Moduł `usb_device` uruchamia urządzenie USB dla WebUSB: konfiguruje źródło zegara USB (HSI48), następnie uruchamia USBD core, rejestruje klasę WebUSB i uruchamia obsługę zdarzeń USB.

## Przepływ logiki (sekwencja inicjalizacji)

Algorytm gdy `USE_WEBUSB=1`:
1. `MX_USB_Device_Init()`:
   - wywołaj `USBD_Clock_Config()`
   - `USBD_Init(&hUsbDeviceFS, &WEBUSB_Desc, USBD_SPEED_FULL)`
   - `USBD_RegisterClass(&hUsbDeviceFS, &USBD_WEBUSB_ClassDriver)`
   - `USBD_Start(&hUsbDeviceFS)`
2. `USBD_Clock_Config()`:
   - wybierz `RCC_PERIPHCLK_USB` i `RCC_USBCLKSOURCE_HSI48`
   - włącz oscylator HSI48
   - sprawdź statusy zwrotu HAL, wywołaj `Error_Handler()` na błędy

## Przerwania i rejestry

Moduł nie zawiera ISR: cała obsługa IRQ delegowana do middleware USB i programów obsługi w `stm32h7xx_it_usb.c`.
Brak bezpośredniej pracy rejestru (tylko via konfiguracja HAL RCC).

## Zależności

Bezpośrednie:
- Konfiguracja zegara HAL RCC
- USBD core ze statusu STM32 USB middleware
- Implementacja sterownika klasy WebUSB
- Programy obsługi przerwań USB (w `stm32h7xx_it_usb.c`)

Pośrednie:
- `usb_webusb.md` (sterownik klasy)
- `task_net.md` (aplikacja może używać USB jako alternatywy dla Ethernet)

## Relacje modułów

- `usb_webusb.md` (implementacja vendor class)
- `stm32h7xx_it_usb.md` (programy obsługi przerwań)
- `hw_init.md` (powiązana konfiguracja zegara)
