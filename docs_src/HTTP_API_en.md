\page http_api_en "HTTP API (Ethernet wallet)"
\related task_net
\related tx_request_validate

# CryptoWallet HTTP API

<brief>Minimal HTTP server on **port 80** (LwIP, `USE_LWIP=1`). Implementation: `Src/task_net.c`. No TLS or HTTP auth — trusted LAN only. User confirmation is on-device (button), not via HTTP.</brief>

**Russian:** [HTTP_API_ru.md](HTTP_API_ru.md)

---

## Summary

| Item | Value |
|------|--------|
| Base | `http://<device-ip>/` |
| Port | **80** |
| Request size | Must fit in one receive buffer (**&lt; 1024** bytes, `REQ_BUF_SIZE`) |
| Recv timeout | **2000 ms** |

---

## Routes

| Method | Path | Status | Response |
|--------|------|--------|----------|
| `GET` | `/ping` | 200 | `text/plain` body: `pong` |
| `GET` | `/` | 200 | `text/html` demo form + script |
| `GET` | `/tx/signed` | 200 | JSON: signature status (see below) |
| `POST` | `/tx` | 200 | HTML “submitted” page (always 200; see caveat) |
| `GET` | other | **404** | minimal 404 |
| `POST` | other | 200 | plain text `OK` (stub; do not rely on it) |

---

## `POST /tx`

**JSON:** include substring `application/json` in headers (e.g. `Content-Type: application/json`). Body keys: `"recipient"`, `"amount"`, `"currency"` (naive string parser — no escape handling).

**Form:** `application/x-www-form-urlencoded` with `recipient=`, `amount=`, `currency=`.

Field sizes: `TX_RECIPIENT_LEN` 42, `TX_AMOUNT_LEN` 24, `TX_CURRENCY_LEN` 8 (see `wallet_shared.h`). Empty `currency` defaults to **`BTC`** in the parser.

Validation: `tx_request_validate()` — see [tx_request_validate.md](tx_request_validate.md).

**Caveat:** HTTP handler always returns **200** with the same HTML success page even if validation fails or enqueue fails — use `GET /tx/signed` or device UI for outcome.

---

## `GET /tx/signed`

```json
{"status":"signed","sig":"<128 hex chars>"}
```

or

```json
{"status":"pending"}
```

---

## Related code

`Src/task_net.c`, `wallet_shared.h`, `tx_request_validate.c`.
