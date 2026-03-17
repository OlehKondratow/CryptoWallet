# API Reference

## Core

### hw_init.h

- `void HW_Init(void)` — Clock, MPU, GPIO, I2C1, UART
- `void HW_Init_Early_LwIP(void)` — MPU + Cache before HAL (LwIP only)
- `void UART_Log(const char *msg)` — Log to UART

### task_net.h

- `void Task_Net_Create(void)` — Create network task (LwIP HTTP server)

### task_display.h

- `void Task_Display_Create(void)` — Create display task
- `void Task_Display_Log(const char *msg)` — Append to display log

### wallet_shared.h

- `wallet_tx_t` — Transaction data (recipient, amount, currency)
- `display_context_t` — Display state (IP, MAC, log, etc.)

---

## HTTP API (task_net, port 80)

| Method | Path       | Content-Type                    | Описание                    |
|--------|------------|---------------------------------|-----------------------------|
| GET    | /ping      | —                               | Тест связи → `pong`        |
| GET    | /          | —                               | HTML-форма для подписи TX   |
| POST   | /tx        | application/json или form-urlencoded | Запрос на подпись транзакции |
| GET    | /tx/signed | —                               | JSON с последней подписью   |

### Поля запроса (POST /tx)

| Поле      | Описание              | Пример                    |
|-----------|------------------------|---------------------------|
| recipient | Адрес получателя      | 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa |
| amount    | Сумма                 | 0.001                     |
| currency  | Валюта (BTC, ETH, …)  | BTC                       |

Приватный ключ выводится из seed на устройстве по пути `m/44'/0'/0'/0/0` (Bitcoin). Поле `key` в API не поддерживается.

### Примеры запросов

**Form-urlencoded (как HTML-форма):**

```bash
curl -X POST http://192.168.1.100/tx \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "recipient=1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa&amount=0.001&currency=BTC"
```

**JSON:**

```bash
curl -X POST http://192.168.1.100/tx \
  -H "Content-Type: application/json" \
  -d '{"recipient":"1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa","amount":"0.001","currency":"BTC"}'
```

**Ответ GET /tx/signed:**

```json
{"status":"signed","sig":"A1B2C3D4..."}
```

или

```json
{"status":"pending"}
```
