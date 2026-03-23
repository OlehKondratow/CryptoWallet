# 4. HTTP i WebUSB

## 4.1 HTTP (Ethernet, LwIP)

**Implementacja:** `Src/task_net.c`. **Port:** 80. **TLS:** brak.

### Podsumowanie

| Pozycja | Wartość |
|---------|---------|
| Baza | `http://<ip-urządzenia>/` |
| Maks. żądanie | Musi zmieścić się w buforze odbioru (rząd 1 KiB; patrz `REQ_BUF_SIZE` w źródle) |
| Timeout odbioru | Rząd 2 s (patrz źródło) |

### Trasy

| Metoda | Ścieżka | Odpowiedź |
|--------|---------|-----------|
| GET | `/ping` | `200` `text/plain`: `pong` |
| GET | `/` | `200` HTML demo |
| GET | `/tx/signed` | `200` JSON status / podpis |
| POST | `/tx` | `200` HTML (patrz uwaga poniżej) |
| * | inne | 404 lub stub |

### `POST /tx`

- **JSON:** `Content-Type` musi zawierać `application/json`. Pola: `recipient`, `amount`, `currency` (naiwny parser).
- **Formularz:** `application/x-www-form-urlencoded`.

Walidacja przez `tx_request_validate()`. **Uwaga:** warstwa HTTP może zwrócić sukces HTML nawet gdy kolejka się nie powiedzie — **zawsze** używaj `/tx/signed` lub UI urządzenia jako źródła prawdy.

### `GET /tx/signed`

Przykłady JSON:

```json
{"status":"signed","sig":"<hex>"}
```

```json
{"status":"pending"}
```

## 4.2 WebUSB

**Pliki:** `Core/Src/usb_device.c`, `usb_webusb.c`, `usbd_conf_cw.c`, `usbd_desc_cw.c`.

- Protokół dostawcy dla operacji portfela obok HTTP.
- Deskryptory zawierają UUID platformy WebUSB (patrz źródła).
- Ten sam model zaufania: **brak** uwierzytelnienia transportu; potwierdzenie na urządzeniu.

## 4.3 Zasada operatora

Traktuj **Ethernet + HTTP** wyłącznie jako **zaufaną LAN**. Brak poufności i uwierzytelnienia żądań w sieci — dostęp fizyczny i miejsce w sieci są częścią modelu zagrożeń.
