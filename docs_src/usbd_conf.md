\page usbd_conf "usbd_conf: USB device config redirect"

# `Core/Inc/usbd_conf.h`

<brief>Заголовок `usbd_conf` является тонкой обёрткой: он подключает `usbd_conf_cw.h`, тем самым “привязывая” параметры USB device middleware к конкретной реализации CryptoWallet WebUSB BSP.</brief>

## Краткий обзор
<brief>Заголовок `usbd_conf` является тонкой обёрткой: он подключает `usbd_conf_cw.h`, тем самым “привязывая” параметры USB device middleware к конкретной реализации CryptoWallet WebUSB BSP.</brief>

## Abstract (Synthèse логики)
USBD middleware ожидает, что проект предоставит конфигурационный header (`usbd_conf.h`). Чтобы избежать дублирования и держать конфигурации “на уровне проекта”, здесь сделан redirect на `usbd_conf_cw.h`, который содержит реальную BSP-логику (MSP init, static buffers, malloc hooks).

## Logic Flow
Это compile-time redirection:
1. Подключить `usbd_conf_cw.h`.
2. Экспортировать его определения наружу.

## Прерывания/регистры
Нет.

## Тайминги
Нет.

## Dependencies
- `usbd_conf_cw.h`

## Связи
- `usbd_conf_cw.md`: реальная конфигурация BSP.
- `usb_device.md` / `usb_webusb.md`: используют USBD middleware и получают конфигурацию через include chain.

