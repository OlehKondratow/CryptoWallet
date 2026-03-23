# 4. HTTP and WebUSB

## 4.1 HTTP (Ethernet, LwIP)

**Implementation:** `Src/task_net.c`. **Port:** 80. **TLS:** none.

### Summary

| Item | Value |
|------|--------|
| Base | `http://<device-ip>/` |
| Max request | Must fit receive buffer (order of 1 KiB; see `REQ_BUF_SIZE` in source) |
| Recv timeout | Order of 2 s (see source) |

### Routes

| Method | Path | Response |
|--------|------|----------|
| GET | `/ping` | `200` `text/plain`: `pong` |
| GET | `/` | `200` HTML demo |
| GET | `/tx/signed` | `200` JSON status / signature |
| POST | `/tx` | `200` HTML (see caveat below) |
| * | other | 404 or stub |

### `POST /tx`

- **JSON:** `Content-Type` must mention `application/json`. Body keys: `recipient`, `amount`, `currency` (naive parser).
- **Form:** `application/x-www-form-urlencoded`.

Validation via `tx_request_validate()`. **Caveat:** HTTP layer may still return success HTML if enqueue fails—**always** use `/tx/signed` or device UI for authoritative state.

### `GET /tx/signed`

JSON examples:

```json
{"status":"signed","sig":"<hex>"}
```

```json
{"status":"pending"}
```

## 4.2 WebUSB

**Files:** `Core/Src/usb_device.c`, `usb_webusb.c`, `usbd_conf_cw.c`, `usbd_desc_cw.c`.

- Vendor-specific protocol for wallet operations complementary to HTTP.
- Descriptors include WebUSB platform UUID (see sources).
- Same trust model: **no** transport authentication; user confirmation on device.

## 4.3 Combined operator rule

Treat **Ethernet + HTTP** as a **trusted LAN** only. There is no on-wire confidentiality or request authentication—physical access and network placement are part of your threat model.
