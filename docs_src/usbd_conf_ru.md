\page usbd_conf "usbd_conf: USB device config редирект"

# `Core/Inc/usbd_conf.h`

<brief>Заголовок `usbd_conf` — тонкая обёртка: он включает `usbd_conf_cw.h`, тем самым "привязывая" конфигурацию USB device middleware к конкретной WebUSB BSP реализации CryptoWallet.</brief>

## Обзор

<brief>Заголовок `usbd_conf` — тонкая обёртка: он включает `usbd_conf_cw.h`, тем самым "привязывая" конфигурацию USB device middleware к конкретной WebUSB BSP реализации CryptoWallet.</brief>

## Абстракция (синтез логики)

USBD middleware ожидает, что проект предоставит конфигурационный заголовок (`usbd_conf.h`). Чтобы избежать дублирования и держать конфигурацию на "уровне проекта", здесь сделан редирект на `usbd_conf_cw.h`, который содержит реальную BSP-логику (MSP init, статические буферы, malloc hooks).

## Поток логики

Это редирект compile-time:
1. Включить `usbd_conf_cw.h`
2. Экспортировать его определения наружу

## Прерывания/регистры

Нет.

## Времена

Нет.

## Зависимости

- `usbd_conf_cw.h`

## Связи

- `usbd_conf_cw.md` — Реальная конфигурация BSP
- `usb_device.md` / `usb_webusb.md` — Используют USBD middleware и получают конфигурацию через цепь include
