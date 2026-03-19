\page usbd_desc_cw "usbd_desc_cw: WebUSB USB дескрипторы + BOS"

# `usbd_desc_cw.c` + `usbd_desc_cw.h`

<brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief>

## Обзор

<brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief>

## Абстракция (синтез логики)

USB-хост для WebUSB должен получить корректный набор дескрипторов устройства, включая информацию, по которой Chrome/host APIs понимают, что это WebUSB девайс. `usbd_desc_cw` решает эту задачу: описывает VID/PID, формирует строки, предоставляет BOS/WebUSB capability, и генерирует серийный номер, чтобы хост различал устройства.

## Поток логики (провайдер дескрипторов)

USBD middleware вызывает через указатели функций в объекте `WEBUSB_Desc`:
1. При запросе device/config/interface/string/BOS дескриптора:
   - Вернуть заранее подготовленные массивы (device/lang/bos и т.п.)
   - Строки manufacturer/product генерируются через `USBD_GetString(...)` на лету
   - Серийная строка генерируется через `Get_SerialNum()`
2. `Get_SerialNum()`:
   - Читает `DEVICE_ID1/2/3`
   - Комбинирует их (d0 += d2)
   - Конвертирует числа в hex-символы через `IntToUnicode()`

Инварианты:

| Параметр | Значение |
|---|---|
| VID/PID | `0x1209 / 0xC0DE` |
| USB скорость | `USBD_SPEED_FULL` (скорость используется в init) |
| WebUSB UUID (BOS capability) | `3408b638-09a9-47a0-8bfd-a0768815b665` (в массиве дескриптора) |

## Прерывания/регистры

Нет ISR. Есть чтение device ID регистров (memory-mapped значения через макро-DEVICE_ID*).

## Времена

Нет специфических временных ограничений: дескрипторы выдаются при подключении/запросах хоста.

## Зависимости

Прямые:
- USBD middleware типы дескрипторов: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_def.h`
- Конфигурация: `usbd_conf_cw.h` (для size/macros)
- Внешнее `WEBUSB_Desc` используется `usb_device.c` при `USBD_Init`

## Связи

- `usb_device.md` — Вызывает `USBD_Init(... &WEBUSB_Desc ...)`
- `usb_webusb.md` — Содержит class/config дескриптор (EP layout)
