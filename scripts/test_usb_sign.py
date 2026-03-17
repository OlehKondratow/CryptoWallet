#!/usr/bin/env python3
"""
Тестовый скрипт для проверки подписи транзакции по WebUSB.

Требования:
  pip install pyusb
  Linux: libusb-1.0 (apt install libusb-1.0-0-dev), udev rules
         udev/99-cryptowallet-webusb.rules → /etc/udev/rules.d/

Использование:
  python scripts/test_usb_sign.py ping          # проверка связи (ping → pong)
  python scripts/test_usb_sign.py sign          # запрос подписи (подтвердить USER на устройстве)
  python scripts/test_usb_sign.py sign --recipient 1A1zP1... --amount 0.001 --currency BTC
"""

import argparse
import json
import sys

try:
    import usb.core
    import usb.util
except ImportError:
    print("Установите pyusb: pip install pyusb")
    sys.exit(1)

# CryptoWallet WebUSB (usbd_desc_cw.c)
VID = 0x1209
PID = 0xC0DE

# Endpoints (usb_webusb.h)
EP_OUT = 0x02  # host -> device
EP_IN = 0x81   # device -> host

# Response types (usb_webusb.c)
CMD_PONG = 0x00
CMD_SIGN_RESP = 0x02

TIMEOUT_MS = 3000
SIGN_MAX_WAIT_S = 30


def find_device():
    dev = usb.core.find(idVendor=VID, idProduct=PID)
    if dev is None:
        raise RuntimeError(f"Устройство {VID:04x}:{PID:04x} не найдено. Подключите CryptoWallet по USB.")
    return dev


def claim_interface(dev):
    if dev.is_kernel_driver_active(0):
        dev.detach_kernel_driver(0)
    cfg = dev.get_active_configuration()
    iface = cfg[(0, 0)]
    usb.util.claim_interface(dev, 0)
    return iface


def release_interface(dev):
    try:
        usb.util.release_interface(dev, 0)
        if dev.is_kernel_driver_active(0) is False:
            dev.attach_kernel_driver(0)
    except usb.core.USBError:
        pass


def write(dev, data: bytes) -> int:
    return dev.write(EP_OUT, data, timeout=TIMEOUT_MS)


def read(dev, size: int = 64) -> bytes:
    return bytes(dev.read(EP_IN, size, timeout=TIMEOUT_MS))


def cmd_ping(dev) -> bool:
    """Ping: отправить 'ping', получить 'pong'."""
    write(dev, b"ping")
    resp = read(dev, 4)
    ok = resp == b"pong"
    print("pong" if ok else f"Ожидалось 'pong', получено: {resp!r}")
    return ok


def cmd_sign(dev, recipient: str, amount: str, currency: str) -> bytes | None:
    """
    Запрос подписи. Отправить JSON, ждать подтверждения на устройстве,
    получить 65 байт (0x02 + 64 байта подписи).
    """
    tx = {
        "recipient": recipient,
        "amount": amount,
        "currency": currency,
    }
    payload = json.dumps(tx).encode("utf-8")
    write(dev, payload)
    print("TX отправлен. Подтвердите на устройстве (короткое нажатие USER)...")

    try:
        resp = dev.read(EP_IN, 65, timeout=SIGN_MAX_WAIT_S * 1000)
    except usb.core.USBError as e:
        if "timed out" in str(e).lower() or "timeout" in str(e).lower():
            print("Таймаут: подпись не получена. Подтвердите на устройстве.")
        else:
            print(f"USB ошибка при чтении: {e}")
        return None

    resp = bytes(resp)
    if len(resp) >= 65 and resp[0] == CMD_SIGN_RESP:
        return resp[1:65]
    print(f"Неожиданный ответ: {resp[:16].hex()}... (len={len(resp)})")
    return None


def main():
    parser = argparse.ArgumentParser(description="Тест подписи CryptoWallet по USB")
    parser.add_argument("cmd", choices=["ping", "sign"], help="ping | sign")
    parser.add_argument("--recipient", default="1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", help="Адрес получателя")
    parser.add_argument("--amount", default="0.001", help="Сумма")
    parser.add_argument("--currency", default="BTC", help="Валюта")
    args = parser.parse_args()

    dev = None
    try:
        dev = find_device()
        dev.set_configuration()
        claim_interface(dev)

        if args.cmd == "ping":
            ok = cmd_ping(dev)
            sys.exit(0 if ok else 1)

        if args.cmd == "sign":
            sig = cmd_sign(dev, args.recipient, args.amount, args.currency)
            if sig is not None:
                hex_sig = sig.hex().upper()
                print(f"Подпись ({len(sig)} байт):\n{hex_sig}")
                sys.exit(0)
            sys.exit(1)

    except usb.core.USBError as e:
        print(f"USB ошибка: {e}")
        if "Access denied" in str(e) or "Permission" in str(e):
            print("Подсказка: добавьте udev rules или запустите с sudo")
        sys.exit(1)
    except RuntimeError as e:
        print(e)
        sys.exit(1)
    finally:
        if dev is not None:
            release_interface(dev)


if __name__ == "__main__":
    main()
