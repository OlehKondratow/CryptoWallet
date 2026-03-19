\page usb_webusb "usb_webusb: WebUSB vendor интерфейс (ping + sign)"
\related WebUSB_NotifySignatureReady
\related usb_webusb

# `usb_webusb.c` + `usb_webusb.h`

<brief>Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`; при готовности подписи отправляет 64-байтовый компактный `r||s` через `WebUSB_NotifySignatureReady()`.</brief>

## Краткий обзор

Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`.

## Логика потока (USB class state + command handling)

### Подготовка класса

Включается только при `USE_WEBUSB=1`:
1. `USBD_WEBUSB_Init()`:
   - сохранить `s_pdev`
   - открыть EP IN (0x81) и EP OUT (0x02) как bulk с MPS = 64 байта
   - постить OUT transfers для приёма команд

2. `USBD_WEBUSB_DataOut()` (OUT endpoint callback):
   - получить command buffer (ping или sign_request frame)
   - распарсить и обработать соответственно

### Обработка команд

| Команда | Input | Output | Действие |
|---------|-------|--------|----------|
| `ping` | 4-байт "PING" | "PONG" | Echo health check |
| `sign_request` | recipient\|amount\|currency bytes | queued to `g_tx_queue` | Validate + enqueue |

## Зависимости

Прямые:
- USBD core и class driver framework
- USB endpoint управление (HAL USB)
- `tx_request_validate()` для валидации
- `g_tx_queue` для queueing транзакций
- `g_last_sig[]` и `g_last_sig_ready` для readout подписи

Косвенные:
- `task_sign.md` (signature generation)
- `usb_device.md` (class registration)

## Связи модулей

- `usb_device.md` (class driver хост)
- `task_sign.md` (signature provider)
- `tx_request_validate.md` (input validation)
- `wallet_shared.md` (transaction contract)
