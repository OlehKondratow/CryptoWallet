\page usb_device "usb_device: WebUSB device init (clock + USBD core)"
\related MX_USB_Device_Init

# `usb_device.c` + `usb_device.h`

<brief>Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий.</brief>

## Краткий обзор
<brief>Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий.</brief>

## Abstract (Synthèse логики)
Встроенная USB функциональность требует корректного тактирования и инициализации USBD-стека до того, как vendor-class `usb_webusb` начнёт принимать данные. `usb_device` — тонкий стартовый слой, который соединяет системные clock-конфигурации с инфраструктурой USB device middleware.

## Logic Flow (init sequence)
Алгоритм при `USE_WEBUSB=1`:
1. `MX_USB_Device_Init()`:
   - вызвать `USBD_Clock_Config()`,
   - `USBD_Init(&hUsbDeviceFS, &WEBUSB_Desc, USBD_SPEED_FULL)`,
   - `USBD_RegisterClass(&hUsbDeviceFS, &USBD_WEBUSB_ClassDriver)`,
   - `USBD_Start(&hUsbDeviceFS)`.
2. `USBD_Clock_Config()`:
   - выбрать `RCC_PERIPHCLK_USB` и `RCC_USBCLKSOURCE_HSI48`,
   - включить HSI48 осциллятор,
   - проверить возврат HAL статусов и при ошибках вызвать `Error_Handler()`.

## Прерывания/регистры
Модуль не содержит ISR: вся обработка IRQ делегируется USB middleware и обработчикам в `stm32h7xx_it_usb.c`.
Прямая работа с регистрами не выполняется (только через HAL RCC конфигурации).

## Тайминги и ветвления
Ветка по сборочному флагу:
| Условие | Поведение |
|---|---|
| `USE_WEBUSB!=1` | инициализация не компилируется |
| HAL init/clock ошибка | `Error_Handler()` |

## Dependencies
Прямые:
- USBD middleware: `usbd_core.h` / `usbd_desc_cw.h` / `usbd_ctlreq.h` через заголовки.
- WebUSB class driver: `usb_webusb.h`.
- USB descriptors: `usbd_desc_cw.h` (объект `WEBUSB_Desc`).

## Связи
- `usbd_conf_cw.md`: BSP для PCD (MSP init) и IRQ bridge.
- `usb_webusb.md`: класс/команды (ping/sign).

