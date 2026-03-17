# Пошаговая проверка подписи транзакции

## Уровень 0: Ping-Pong (проверка связи)

Перед тестом подписи убедитесь, что канал связи работает.

### HTTP

```bash
curl http://<IP>/ping
```

Ожидание: `pong`

### WebUSB

В консоли браузера (Chrome, localhost или HTTPS):

```javascript
const device = await navigator.usb.requestDevice({ filters: [{ vendorId: 0x1209, productId: 0xC0DE }] });
await device.open();
await device.claimInterface(0);
await device.transferOut(2, new TextEncoder().encode("ping"));
const r = await device.transferIn(1, 64);
console.log(new TextDecoder().decode(r.data));  // "pong"
```

В UART: `WebUSB ping`

---

## Подготовка

### 1. Сборка и прошивка

```bash
make USE_WEBUSB=1 USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 minimal-lwip
make flash-minimal-lwip
```

### 2. Подключение

| Что | Куда |
|-----|------|
| Ethernet | LAN8742 (разъём на Nucleo) |
| USB ST-Link | ПК (для UART-логов) |
| USB WebUSB (опционально) | CN13 (PA11/PA12) — второй USB |

**UART**: ST-Link VCP (COM-порт в Windows, `/dev/ttyACM*` в Linux). 115200 8N1.

---

## Шаг 1: Загрузка и сеть

**Ожидание**: устройство загружается, получает IP по DHCP.

**Проверка**:
- Подключить `screen /dev/ttyACM0 115200` (или PuTTY)
- В логах UART должны появиться:
  ```
  CryptoWallet + LWIP
  main: queues OK
  main: tasks created, starting scheduler
  Sign task init
  User btn init
  Disp start
  Net: start
  Net: tcpip_init...
  Net: tcpip OK
  [ETH] Link up
  [DHCP] Start
  Net: HTTP ready
  ```

**LED**: LED1 (зелёный) — мигает при работе.

---

## Шаг 2: Отправка запроса на подпись

**Действие**: отправить POST /tx (HTTP или WebUSB).

**HTTP** (узнать IP из DHCP или использовать 192.168.0.10 при статике):

```bash
curl -X POST http://<IP>/tx \
  -H "Content-Type: application/json" \
  -d '{"recipient":"1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa","amount":"0.001","currency":"BTC"}'
```

**Ожидание в UART**:
```
TX enqueued
TX recv
1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

Если вместо этого `TX invalid` — проверьте формат JSON (recipient, amount, currency).

---

## Шаг 3: Ожидание подтверждения

**Состояние**: task_sign ждёт нажатия USER (PC13). Таймаут 30 с.

**UART**: новых сообщений нет до нажатия.

**Действия**:
- **Подтвердить**: короткое нажатие USER (< 2.5 с)
- **Отклонить**: долгое нажатие (≥ 2.5 с)

---

## Шаг 4: Подтверждение (короткое нажатие)

**Ожидание в UART**:
```
Confirm
```

Далее task_sign:
1. Вызывает `get_wallet_seed()` → seed
2. `crypto_derive_btc_m44_0_0_0_0()` → priv_key
3. `crypto_sign_btc_hash()` → sig
4. Записывает в `g_last_sig`, вызывает `WebUSB_NotifySignatureReady()` (если USE_WEBUSB)

**Возможные сообщения**:

| Сообщение | Причина |
|-----------|---------|
| `Signed OK` | Подпись успешна |
| `No seed` | `get_wallet_seed()` вернул -1 → собрать с USE_TEST_SEED=1 |
| `Derive err` | Ошибка BIP-32 |
| `Sign err` | Ошибка ECDSA |

---

## Шаг 5: Получение подписи

**HTTP**:
```bash
curl http://<IP>/tx/signed
```

Ожидаемый ответ:
```json
{"status":"signed","sig":"<hex 64 bytes>"}
```

**WebUSB**: `transferIn(1, 65)` — 1 байт (0x02) + 64 байта подписи.

---

## Шаг 6: Отклонение (опционально)

Повторить шаги 1–2, затем **долгое нажатие** USER (≥ 2.5 с).

**Ожидание в UART**:
```
Reject
```

Подпись не выполняется, `g_last_sig` не обновляется.

---

## Шаг 7: Таймаут (опционально)

Повторить шаги 1–2, **не нажимать** USER 30 секунд.

**Ожидание в UART**:
```
Timeout
```

---

## Краткий чеклист

- [ ] Сборка с USE_TEST_SEED=1
- [ ] UART 115200 — видны логи загрузки
- [ ] Ethernet link up, DHCP OK
- [ ] POST /tx → `TX enqueued`, `TX recv`, адрес в логе
- [ ] Короткое нажатие USER → `Confirm` → `Signed OK`
- [ ] GET /tx/signed → `{"status":"signed","sig":"..."}`
- [ ] Долгое нажатие → `Reject`
- [ ] Без нажатия 30 с → `Timeout`

---

## Отладка через GDB

При подключении OpenOCD + GDB можно ставить breakpoint:

```
break task_sign.c:99   # после получения из очереди
break task_sign.c:137  # после хеширования
break task_sign.c:160  # перед get_wallet_seed
break task_sign.c:179  # после derive
break task_sign.c:189  # после sign
break task_sign.c:200  # Signed OK
```

Команды: `continue`, `next`, `print tx`, `print vr` и т.д.
