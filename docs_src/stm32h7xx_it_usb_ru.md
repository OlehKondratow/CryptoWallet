\page stm32h7xx_it_usb "stm32h7xx_it_usb: OTG_HS IRQ → HAL_PCD_IRQHandler"

# `stm32h7xx_it_usb.c`

<brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: ISR вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог правильно обслуживать transfers endpoint'ов.</brief>

## Обзор

<brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: ISR вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог правильно обслуживать transfers endpoint'ов.</brief>

## Абстракция (синтез логики)

USB transfers и события link/endpoint changes приходят через IRQ. В проекте класс WebUSB живёт поверх STM32 USB device middleware, а middleware ожидает, что определённый IRQ будет маршрутизирован в HAL/PCD обработчики. Этот файл обеспечивает именно такую маршрутизацию: OTG_HS IRQ → HAL handler.

## Поток логики (маршрутизация ISR)

1. При условной компиляции (`USE_WEBUSB==1`) объявлен внешний handle `hpcd_USB_FS`
2. `OTG_HS_IRQHandler()`:
   - Вызвать `HAL_PCD_IRQHandler(&hpcd_USB_FS)`

## Прерывания/регистры

Это ISR. Регистры напрямую не трогаются; HAL забирает ответственность за очистку IRQ флагов и продвижение state машинки USB middleware.

## Времена

ISR должен быть коротким: здесь только один вызов в HAL.

## Зависимости

- STM32 HAL USB device: `HAL_PCD_IRQHandler`
- Обработчик ожидается настроенным в `usbd_conf_cw.c` (NVIC enable/priority)

## Связи

- `usbd_conf_cw.md` — Настраивает IRQ
- `usb_webusb.md` — Получает DataOut/DataIn через middleware
