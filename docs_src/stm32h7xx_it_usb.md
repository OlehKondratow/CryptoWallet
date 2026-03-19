\page stm32h7xx_it_usb "stm32h7xx_it_usb: OTG_HS IRQ -> HAL_PCD_IRQHandler"

# `stm32h7xx_it_usb.c`

<brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: в ISR он вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог корректно обслуживать transfer’ы endpoint’ов.</brief>

## Краткий обзор
<brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: в ISR он вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог корректно обслуживать transfer’ы endpoint’ов.</brief>

## Abstract (Synthèse логики)
USB transfer’ы и события link/endpoint changes приходят через IRQ. В проекте WebUSB класс живёт поверх STM32 USB device middleware, а middleware ожидает, что конкретный IRQ будет маршрутизирован в HAL/PCL обработчики. Этот файл обеспечивает именно такую связку: OTG_HS IRQ -> HAL handler.

## Logic Flow (ISR routing)
1. В условной компиляции (`USE_WEBUSB==1`) объявлен внешний handle `hpcd_USB_FS`.
2. `OTG_HS_IRQHandler()`:
   - вызвать `HAL_PCD_IRQHandler(&hpcd_USB_FS)`.

## Прерывания/регистры
Это ISR. Регистры напрямую не трогаются; HAL забирает ответственность за очистку IRQ флагов и продвигает state машинку USB middleware.

## Тайминги
ISR должен быть коротким: здесь только один вызов в HAL.

## Dependencies
- STM32 HAL USB device: `HAL_PCD_IRQHandler`
- Обработчик ожидается настроенным в `usbd_conf_cw.c` (NVIC enable/priority).

## Связи
- `usbd_conf_cw.md` настраивает IRQ.
- `usb_webusb.md` получает DataOut/DataIn через middleware.

