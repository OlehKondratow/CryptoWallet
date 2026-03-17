# Ручное тестирование подписи транзакций

## Поток подписи

```
HTTP POST /tx  или  WebUSB transferOut
        │
        ▼
   g_tx_queue (wallet_tx_t)
        │
        ▼
   task_sign: валидация (tx_request_validate)
        │
        ▼
   SHA-256(recipient|amount|currency)
        │
        ▼
   Ожидание подтверждения пользователя (USER key PC13)
        │
        ├── Короткое нажатие  → EVENT_USER_CONFIRMED
        └── Долгое (2.5 с)   → EVENT_USER_REJECTED
        │
        ▼ (при Confirm)
   get_wallet_seed() → seed
        │
        ▼
   crypto_derive_btc_m44_0_0_0_0(seed) → priv_key
        │
        ▼
   crypto_sign_btc_hash(priv_key, digest) → sig[64]
        │
        ▼
   g_last_sig, WebUSB_NotifySignatureReady() (если USE_WEBUSB)
```

## Шаги ручного теста

### 1. Сборка с подписью и WebUSB

```bash
# С тестовым seed (для проверки подписи):
make USE_WEBUSB=1 USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 minimal-lwip

# Без seed (подпись будет падать на «No seed»):
make USE_WEBUSB=1 USE_CRYPTO_SIGN=1 minimal-lwip

make flash-minimal-lwip
```

### 2. Отправка запроса на подпись

**Через HTTP (Ethernet):**

```bash
curl -X POST http://<IP_устройства>/tx \
  -H "Content-Type: application/json" \
  -d '{"recipient":"1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa","amount":"0.001","currency":"BTC"}'
```

**Через WebUSB (Chrome):**

1. Открыть HTML-демо из `docs_src/webusb.md` (localhost или HTTPS)
2. Нажать Connect → выбрать устройство
3. Нажать Sign TX

**Через WebUSB (Python):**

```bash
pip install pyusb
python scripts/test_usb_sign.py ping   # проверка связи
python scripts/test_usb_sign.py sign    # запрос подписи
```

### 3. Подтверждение на устройстве

- **Короткое нажатие** USER (PC13) — подтвердить подпись
- **Долгое нажатие** (2.5 с) — отклонить

### 4. Получение подписи

**HTTP:** `GET http://<IP>/tx/signed` — JSON с полем `sig`.

**WebUSB:** `transferIn(1, 65)` — 1 байт (0x02) + 64 байта подписи.

## Текущее ограничение: No seed

По умолчанию `get_wallet_seed()` возвращает `-1` (seed не реализован). В этом случае после Confirm на дисплее появится «No seed», подпись не выполнится.

Чтобы тест прошёл успешно, соберите с **USE_TEST_SEED=1** (тестовая мнемоника «abandon … about»):

```bash
make USE_WEBUSB=1 USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 minimal-lwip
```

Или реализуйте `get_wallet_seed()` в своём модуле (см. [Добавление кошелька](wallet-setup.md)).

---

## Кнопка USER (NUCLEO-H743ZI2)

| Пин  | Функция      |
|------|--------------|
| PC13 | USER (синяя) |

- Короткое нажатие (< 2.5 с) → Confirm
- Долгое нажатие (≥ 2.5 с) → Reject
