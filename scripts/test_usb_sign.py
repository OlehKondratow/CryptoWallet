#!/usr/bin/env python3
"""
Test script for transaction signing over WebUSB.

Documentation index: README.md (repo root). Protocol notes: Core/Src/usb_webusb.c , docs_src/webusb.md .

Requirements:
  pip install -r scripts/requirements.txt   # pyusb (+ pyserial for other scripts)
  Linux: libusb-1.0 (apt install libusb-1.0-0-dev), udev rules
         udev/99-cryptowallet-webusb.rules → /etc/udev/rules.d/

Usage:
  python3 scripts/test_usb_sign.py ping          # connectivity (ping → pong)
  python3 scripts/test_usb_sign.py sign          # signing request (confirm USER on device)
  python3 scripts/test_usb_sign.py sign --recipient 1A1zP1... --amount 0.001 --currency BTC
"""

import argparse
import json
import sys

try:
    import usb.core
    import usb.util
except ImportError:
    print("Install pyusb: pip install -r scripts/requirements.txt")
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
        raise RuntimeError(
            f"Device {VID:04x}:{PID:04x} not found. Connect CryptoWallet via USB."
        )
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
    """Ping: send 'ping', expect 'pong'."""
    write(dev, b"ping")
    resp = read(dev, 4)
    ok = resp == b"pong"
    print("pong" if ok else f"Expected 'pong', got: {resp!r}")
    return ok


def cmd_sign(dev, recipient: str, amount: str, currency: str) -> bytes | None:
    """
    Signing request: send JSON, wait for confirmation on device,
    receive 65 bytes (0x02 + 64-byte signature).
    """
    tx = {
        "recipient": recipient,
        "amount": amount,
        "currency": currency,
    }
    payload = json.dumps(tx).encode("utf-8")
    write(dev, payload)
    print("TX sent. Confirm on device (short USER press)...")

    try:
        resp = dev.read(EP_IN, 65, timeout=SIGN_MAX_WAIT_S * 1000)
    except usb.core.USBError as e:
        if "timed out" in str(e).lower() or "timeout" in str(e).lower():
            print("Timeout: no signature. Confirm on the device.")
        else:
            print(f"USB read error: {e}")
        return None

    resp = bytes(resp)
    if len(resp) >= 65 and resp[0] == CMD_SIGN_RESP:
        return resp[1:65]
    print(f"Unexpected response: {resp[:16].hex()}... (len={len(resp)})")
    return None


def main():
    parser = argparse.ArgumentParser(description="CryptoWallet USB signing test")
    parser.add_argument("cmd", choices=["ping", "sign"], help="ping | sign")
    parser.add_argument(
        "--recipient",
        default="1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
        help="Recipient address",
    )
    parser.add_argument("--amount", default="0.001", help="Amount")
    parser.add_argument("--currency", default="BTC", help="Currency")
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
                print(f"Signature ({len(sig)} bytes):\n{hex_sig}")
                sys.exit(0)
            sys.exit(1)

    except usb.core.USBError as e:
        print(f"USB error: {e}")
        if "Access denied" in str(e) or "Permission" in str(e):
            print("Hint: add udev rules or run with sudo")
        sys.exit(1)
    except RuntimeError as e:
        print(e)
        sys.exit(1)
    finally:
        if dev is not None:
            release_interface(dev)


if __name__ == "__main__":
    main()
