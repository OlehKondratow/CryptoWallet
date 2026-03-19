\page usb_webusb "usb_webusb: WebUSB vendor interface (ping + sign)"
\related WebUSB_NotifySignatureReady
\related usb_webusb

# `usb_webusb.c` + `usb_webusb.h`

<brief>Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`; при готовности подписи отправляет 64-байтовый компактный `r||s` через `WebUSB_NotifySignatureReady()`.</brief>

## Краткий обзор
<brief>Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`; при готовности подписи отправляет 64-байтовый компактный `r||s` через `WebUSB_NotifySignatureReady()`.</brief>

## Abstract (Synthèse логики)
WebUSB в этом проекте — альтернативный канал ввода/вывода вместо HTTP. Его роль — предоставить простой и быстрый способ хосту:
1) проверить доступность устройства (`ping`),
2) передать параметры для подписания (payload, похожий на JSON по подстрокам),
3) получить результат подписи в машинном виде, когда `task_sign` закончит.

Модуль специально избегает тяжёлых парсеров: он решает задачу на “строковых эвристиках” (search + extract), после чего всё равно пропускает данные через `tx_request_validate` перед тем, как отправить в очередь подписания.

## Logic Flow (USB class state + command handling)
### Подготовка класса
Включается только при `USE_WEBUSB=1`:
1. `USBD_WEBUSB_Init()`:
   - сохранить `s_pdev`,
   - открыть EP IN (0x81) и EP OUT (0x02) как bulk с MPS = 64 байта,
   - подготовить первый receive на EP OUT в `s_rx_buf`.

### Обработка входящих данных (`DataOut`)
2. `USBD_WEBUSB_DataOut()`:
   - если epnum не соответствует OUT endpoint — игнор,
   - определить фактическую длину приёма (`USBD_LL_GetRxDataSize`),
   - всегда подготовить следующий `PrepareReceive`,
   - если длина в допустимом диапазоне — вызвать `process_sign_request(s_rx_buf, len)`.

### Команды (`process_sign_request`)
3. Фреймирование:
| Команда/формат | Условие | Результат |
|---|---|---|
| `ping` | `len==4` и bytes == `"ping"` | отправить `"pong"` (4 байта) в EP IN |
| `sign request` | минимум `len>=10` | попытаться извлечь поля по подстрокам |

4. Извлечение `wallet_tx_t` из payload:
   - `recipient`, `amount`, `currency` ищутся как `"recipient"`, `"amount"`, `"currency"` и затем вырезаются между кавычками после двоеточия.
   - после извлечения выставляется дефолт `"BTC"`, если currency пустой.

5. Валидирование и enqueue:
   - вызов `tx_request_validate(&tx)`,
   - при `TX_VALID_OK` и наличии `g_tx_queue` транзакция отправляется в очередь с timeout `100ms`,
   - логирование `"WebUSB TX"`.

### Отправка подписи на хост
6. `WebUSB_NotifySignatureReady()`:
   - вызывается из уровня подписания (в проекте — из `task_sign` когда `g_last_sig_ready` установлен),
   - если device подготовлен и `g_last_sig_ready==1`:
     - EP IN отправляет 65 байт: `[CMD_SIGN_RESP=0x02] + 64 байта signature`.

## Прерывания/регистры
Этот модуль не содержит ISR напрямую. Он реагирует на USB-контекст через хуки USBD (DataOut/EP setup), но низкоуровневые IRQ обрабатываются в `stm32h7xx_it_usb.c` и в STM32 USB BSP (`usbd_conf_cw.c`).

## Тайминги и ветвления
| Условие | Тайминг | Поведение |
|---|---:|---|
| len == 4 и `"ping"` | сразу | send pong |
| len < 10 | сразу | ignore payload |
| `xQueueSend(g_tx_queue)` | `pdMS_TO_TICKS(100)` | если очередь занята — payload не уйдёт на подпись |
| `g_last_sig_ready` | проверка | подпись отправляется только когда она свежая |

## Dependencies
Прямые зависимости:
- USB stack/HAL: `usbd_ioreq`, `usbd_ctlreq.h`, `USBD_LL_*`.
- Контракт данных: `wallet_shared.h` (`wallet_tx_t`), `g_tx_queue`.
- Валидация: `tx_request_validate.h`.
- Лог/UX: `task_display.h` (для `Task_Display_Log`).

Косвенные:
- `task_sign` производит `g_last_sig` и устанавливает `g_last_sig_ready`.

## Связи
- `task_sign.md` (генерация подписи + notify)
- `tx_request_validate.md` (валидация host payload)
- `stm32h7xx_it_usb.md` и `usbd_conf_cw.md` (низкоуровневые USB IRQ/BSP)

