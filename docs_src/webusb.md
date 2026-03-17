# WebUSB — прямое USB-подключение к браузеру

Реализация WebUSB для CryptoWallet: устройство подключается к Chrome по USB и доступно через WebUSB API без драйверов.

## Архитектура

```
┌─────────────────┐     USB      ┌──────────────────┐     WebUSB API     ┌─────────────┐
│  CryptoWallet    │◄─────────────►│  Chrome/Brave    │◄──────────────────►│  dApp /    │
│  (STM32H743)     │   PA11/PA12   │  (WinUSB)        │  requestDevice()   │  MetaMask   │
└─────────────────┘               └──────────────────┘                    └─────────────┘
```

## Требования к устройству

1. **Vendor-specific интерфейс** (Class 0xFF) — не занят системным драйвером
2. **Bulk endpoints** — IN и OUT по 64 байта (Full-Speed)
3. **BOS descriptor** — WebUSB Platform Capability (UUID 3408b638-09a9-47a0-8bfd-a0768815b665)
4. **GET_URL** — vendor request для landing page (опционально)
5. **Microsoft OS 2.0** — для Windows (WinUSB без INF)

## Дескрипторы

### Device Descriptor

| Поле        | Значение   |
|-------------|------------|
| idVendor    | 0x1209 (PID.codes) или свой |
| idProduct   | 0xC0DE (пример) |
| bcdDevice   | 0x0100     |

### Configuration — один vendor-specific интерфейс

| Интерфейс | Class | SubClass | Protocol | Endpoints |
|-----------|-------|----------|----------|-----------|
| 0         | 0xFF  | 0x00     | 0x00     | EP1 IN (Bulk 64), EP2 OUT (Bulk 64) |

### BOS + WebUSB Platform Capability

```
BOS descriptor:
  05 0F 1D 00 01     ; bLength, bDescriptorType, wTotalLength, bNumDeviceCaps
  ; WebUSB Platform Capability:
  18 10 05 00        ; bLength, bDescriptorType, bDevCapabilityType, bReserved
  38 B6 08 34 A9 09 A0 47 8B FD A0 76 88 15 B6 65  ; UUID (little-endian)
  00 01              ; bcdVersion 1.0
  01                 ; bVendorCode (для GET_URL)
  01                 ; iLandingPage (индекс URL)
```

### URL Descriptor (GET_URL, wIndex=2)

Схема: 0x01 = https, 0x02 = http. Пример `https://example.com/cryptowallet`:

```
0D 03 01 65 78 61 6D 70 6C 65 2E 63 6F 6D 2F  ...
; bLength, URL(0x03), scheme(https), "example.com/cryptowallet"
```

## Vendor requests

| bRequest | wValue | wIndex | Описание |
|----------|--------|--------|----------|
| 0x01     | iUrl   | 0x02   | GET_URL — вернуть URL descriptor |

## Протокол данных (bulk)

Формат пакетов — TLV или фиксированные команды. Пример для подписи:

```
[CMD: 1 byte][LEN: 2 bytes][DATA...]
0x01 = Sign request (recipient|amount|currency)
0x02 = Signature response (64 bytes)
```

## Пины (NUCLEO-H743ZI2)

| Функция | Пин  |
|---------|------|
| USB_DM  | PA11 |
| USB_DP  | PA12 |

Alternate function: GPIO_AF10_OTG1_FS

## Ping-Pong (проверка связи)

Перед подписью проверьте канал:

```javascript
await device.transferOut(2, new TextEncoder().encode("ping"));
const r = await device.transferIn(1, 64);
console.log(new TextDecoder().decode(r.data));  // "pong"
```

---

## Браузер: пример JavaScript

```javascript
// Запрос устройства (только HTTPS или localhost)
const device = await navigator.usb.requestDevice({
  filters: [{ vendorId: 0x1209, productId: 0xC0DE }]
});

await device.open();
const iface = device.configuration.interfaces[0];
await device.claimInterface(iface.interfaceNumber);

// Отправка данных
const data = new TextEncoder().encode(JSON.stringify({
  recipient: "1A1zP1...",
  amount: "0.001",
  currency: "BTC"
}));
await device.transferOut(2, data);  // EP OUT

// Приём ответа (подпись)
const result = await device.transferIn(1, 64);  // EP IN
const signature = new Uint8Array(result.data.buffer);
```

## Платформы

| ОС        | Драйвер      | Примечание |
|-----------|--------------|------------|
| Windows   | WinUSB       | Microsoft OS 2.0 descriptor или Zadig |
| Linux     | —            | udev: GROUP="plugdev" для VID:PID |
| macOS     | —            | Работает без доп. настроек |
| ChromeOS  | —            | Встроенная поддержка |

## udev (Linux)

```
SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="c0de", MODE="0666"
```

Или с группой: `GROUP="plugdev"`

## Связь с MetaMask

MetaMask не использует WebUSB напрямую для custom устройств. Варианты:

1. **Свой dApp** — страница с WebUSB + UI для подписи
2. **WalletConnect** — bridge между устройством и dApp (отдельная задача)
3. **Совместимость с Ledger/Trezor** — реализация их протокола (сложно)

Текущая реализация — **свой dApp**: страница подключается по WebUSB, отправляет запрос на подпись, получает подпись.

---

## Сборка с WebUSB

```bash
make USE_WEBUSB=1 minimal-lwip
# или с подписью: make USE_WEBUSB=1 USE_CRYPTO_SIGN=1 minimal-lwip
```

Требуется:
- STM32CubeH7: `hal_pcd.c`, `hal_pcd_ex.c`, `ll_usb.c`
- USB Device Library: `usbd_core.c`, `usbd_ctlreq.c`, `usbd_ioreq.c`
- CryptoWallet: `usbd_conf_cw.c`, `usbd_desc_cw.c`, `usb_device.c`, `usb_webusb.c`

Пины: PA11 (USB_DM), PA12 (USB_DP) — CN13 на NUCLEO-H743ZI2.

---

## HTML-демо (localhost или HTTPS)

```html
<!DOCTYPE html>
<html>
<head><meta charset=utf-8><title>CryptoWallet WebUSB</title></head>
<body>
<h1>CryptoWallet WebUSB</h1>
<button id=connect>Connect</button>
<button id=sign disabled>Sign TX</button>
<pre id=log></pre>
<script>
const log = s => { document.getElementById('log').textContent += s + '\n'; };
let device = null, iface = null;

document.getElementById('connect').onclick = async () => {
  try {
    device = await navigator.usb.requestDevice({
      filters: [{ vendorId: 0x1209, productId: 0xC0DE }]
    });
    await device.open();
    iface = device.configuration.interfaces[0];
    await device.claimInterface(iface.interfaceNumber);
    log('Connected');
    document.getElementById('sign').disabled = false;
  } catch (e) { log('Error: ' + e.message); }
};

document.getElementById('sign').onclick = async () => {
  if (!device) return;
  const tx = JSON.stringify({
    recipient: "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
    amount: "0.001",
    currency: "BTC"
  });
  await device.transferOut(2, new TextEncoder().encode(tx));
  log('TX sent — confirm on device');
  const r = await device.transferIn(1, 65);
  if (r.status === 'ok' && r.data) {
    const sig = new Uint8Array(r.data.buffer, r.data.byteOffset, 64);
    log('Sig: ' + Array.from(sig).map(b=>b.toString(16).padStart(2,'0')).join(''));
  }
};
</script>
</body>
</html>
```
