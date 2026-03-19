\page usbd_desc_cw "usbd_desc_cw: WebUSB USB descriptors + BOS"

# `usbd_desc_cw.c` + `usbd_desc_cw.h`

<brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief>

## Краткий обзор
<brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief>

## Abstract (Synthèse логики)
USB-host для WebUSB должен получить корректный набор дескрипторов устройства, включая сведения, по которым Chrome/host API понимают, что это WebUSB девайс. `usbd_desc_cw` решает задачу: описывает VID/PID, формирует строки, предоставляет BOS/WebUSB capability, и генерирует serial number, чтобы хост различал устройства.

## Logic Flow (descriptor provider)
Вызов USBD middleware идёт через функции-указатели в объекте `WEBUSB_Desc`:
1. При запросе device/config/interface/string/BOS дескриптора:
   - вернуть заранее подготовленные массивы (device/lang/bos и т.п.),
   - строки manufacturer/product генерируются через `USBD_GetString(...)` на лету,
   - serial string генерируется через `Get_SerialNum()`.
2. `Get_SerialNum()`:
   - читает `DEVICE_ID1/2/3`,
   - комбинирует их (d0 += d2),
   - конвертирует числа в hex-символы через `IntToUnicode()`.

Инварианты:
| Параметр | Значение |
|---|---|
| VID/PID | `0x1209 / 0xC0DE` |
| USB speed | `USBD_SPEED_FULL` (скорость используется в init) |
| WebUSB UUID (BOS capability) | `3408b638-09a9-47a0-8bfd-a0768815b665` (в массиве дескриптора) |

## Прерывания/регистры
Нет ISR. Есть чтение device ID регистров (memory-mapped значения через макро-DEVICE_ID*).

## Тайминги
Нет специфических временных ограничений: дескрипторы выдаются при подключении/запросах host.

## Dependencies
Прямые:
- USBD middleware дескрипторные типы: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_def.h`
- Конфигурации: `usbd_conf_cw.h` (для size/macros)
- Внешнее `WEBUSB_Desc` используется `usb_device.c` при `USBD_Init`.

## Связи
- `usb_device.md` вызывает `USBD_Init(... &WEBUSB_Desc ...)`.
- `usb_webusb.md` содержит class/config descriptor (EP layout).

