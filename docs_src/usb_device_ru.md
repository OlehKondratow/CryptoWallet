\page usb_device "usb_device: инициализация WebUSB устройства (такт + USBD core)"
\related MX_USB_Device_Init

# `usb_device.c` + `usb_device.h`

<brief>Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий.</brief>

## Краткий обзор

Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий.

## Логика потока (инициализационная последовательность)

Алгоритм при `USE_WEBUSB=1`:
1. `MX_USB_Device_Init()`:
   - вызвать `USBD_Clock_Config()`
   - `USBD_Init(&hUsbDeviceFS, &WEBUSB_Desc, USBD_SPEED_FULL)`
   - `USBD_RegisterClass(&hUsbDeviceFS, &USBD_WEBUSB_ClassDriver)`
   - `USBD_Start(&hUsbDeviceFS)`
2. `USBD_Clock_Config()`:
   - выбрать `RCC_PERIPHCLK_USB` и `RCC_USBCLKSOURCE_HSI48`
   - включить HSI48 осциллятор
   - проверить возврат HAL статусов и при ошибках вызвать `Error_Handler()`

## Прерывания и регистры

Модуль не содержит ISR: вся обработка IRQ делегируется USB middleware и обработчикам в `stm32h7xx_it_usb.c`.
Прямая работа с регистрами не выполняется (только через HAL RCC конфигурации).

## Зависимости

Прямые:
- HAL RCC clock configuration
- USBD core из STM32 USB middleware
- WebUSB class driver реализация
- USB interrupt handlers (в `stm32h7xx_it_usb.c`)

Косвенные:
- `usb_webusb.md` (class driver)
- `task_net.md` (приложение может использовать USB как альтернативу Ethernet)

## Связи модулей

- `usb_webusb.md` (реализация vendor class)
- `stm32h7xx_it_usb.md` (обработчики interrupt)
- `hw_init.md` (связанная clock setup)
