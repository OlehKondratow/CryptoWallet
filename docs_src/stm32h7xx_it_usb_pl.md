\page stm32h7xx_it_usb "stm32h7xx_it_usb: OTG_HS IRQ → HAL_PCD_IRQHandler"

# `stm32h7xx_it_usb.c`

<brief>Plik `stm32h7xx_it_usb` implementuje procedurę obsługi przerwania OTG HS dla trybu WebUSB: ISR wywołuje `HAL_PCD_IRQHandler` dla `hpcd_USB_FS`, aby oprogramowanie pośrednie USB device mogło prawidłowo obsługiwać transfery endpoint'ów.</brief>

## Przegląd

<brief>Plik `stm32h7xx_it_usb` implementuje procedurę obsługi przerwania OTG HS dla trybu WebUSB: ISR wywołuje `HAL_PCD_IRQHandler` dla `hpcd_USB_FS`, aby oprogramowanie pośrednie USB device mogło prawidłowo obsługiwać transfery endpoint'ów.</brief>

## Abstrakcja (synteza logiki)

Transfery USB i zdarzenia zmian łącza/endpoint'u przychodzą poprzez IRQ. W tym projekcie klasa WebUSB znajduje się na górze oprogramowania pośredniego STM32 USB device, a oprogramowanie pośrednie oczekuje, że określony IRQ będzie kierowany do procedur obsługi HAL/PCD. Ten plik zapewnia właśnie takie kierowanie: OTG_HS IRQ → procedura obsługi HAL.

## Przepływ logiki (kierowanie ISR)

1. Przy warunkowej kompilacji (`USE_WEBUSB==1`) zadeklarowany jest zewnętrzny uchwyt `hpcd_USB_FS`
2. `OTG_HS_IRQHandler()`:
   - Wywołaj `HAL_PCD_IRQHandler(&hpcd_USB_FS)`

## Przerwania/rejestry

To jest ISR. Rejestry nie są bezpośrednio manipulowane; HAL przejmuje odpowiedzialność za czyszczenie flag IRQ i zaawansowanie automatu stanów oprogramowania pośredniego USB.

## Czasy

ISR powinno być krótkie: tutaj tylko jedno wywołanie HAL.

## Zależności

- STM32 HAL USB device: `HAL_PCD_IRQHandler`
- Procedura obsługi oczekiwana do konfiguracji w `usbd_conf_cw.c` (włączenie NVIC/priorytet)

## Relacje

- `usbd_conf_cw.md` — Konfiguruje IRQ
- `usb_webusb.md` — Otrzymuje DataOut/DataIn poprzez oprogramowanie pośrednie
