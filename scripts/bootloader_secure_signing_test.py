#!/usr/bin/env python3
"""
Boot + secure signing smoke test for CryptoWallet (NUCLEO-H743ZI2).

Documentation index: repository README.md §2 (scripts) and §1 (firmware modules).

Context
-------
CryptoWallet is usually linked as a **single application** at flash base **0x08000000**
(linker/RTOS pieces come from sibling repo ``stm32_secure_boot``). This script does **not**
implement a custom MCU bootloader; it automates:

1. **Build** — ``minimal-lwip`` with signing + test seed (and optional WebUSB).
2. **Flash** — ``st-flash`` to 0x08000000 (same as ``make flash-minimal-lwip``).
3. **Boot / reachability** — HTTP ``GET /ping`` or UART line wait (optional).
4. **Signing** — ``POST /tx`` → you confirm on USER (PC13) → ``GET /tx/signed``.
5. **Protected-build sanity** — optional scan of ``.elf`` for obvious test-secret strings.

For a **separate** secure-bootloader project (verified boot, RDP, option bytes), use
``../stm32_secure_boot`` and its docs/scripts; flash this app only after your boot policy
allows it.

Dependencies: Python 3.8+ (stdlib). Optional: ``pyserial`` for ``--uart-wait``.

Usage
-----
  # Full flow (Ethernet + HTTP); tries common static IPs if --ip omitted
  python3 scripts/bootloader_secure_signing_test.py

  # Already built & flashed; device at fixed IP
  python3 scripts/bootloader_secure_signing_test.py --no-build --no-flash --ip 192.168.0.10

  # WebUSB signing only (still builds with USE_WEBUSB=1 unless --no-build)
  python3 scripts/bootloader_secure_signing_test.py --webusb --no-flash

  # Only check .elf for test mnemonic leakage (after build)
  python3 scripts/bootloader_secure_signing_test.py --elf-audit-only --no-flash --no-sign
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path


FLASH_ADDR = "0x08000000"
DEFAULT_IP_CANDIDATES = (
    "192.168.0.10",  # static fallback in firmware docs
    "192.168.1.100",
    "192.168.0.100",
)


def project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def run_make(root: Path, extra: list[str]) -> None:
    cmd = ["make", "-C", str(root), "USE_CRYPTO_SIGN=1", "USE_TEST_SEED=1"] + extra + ["minimal-lwip"]
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def run_flash(root: Path) -> None:
    bin_path = root / "build" / "cryptowallet.bin"
    if not bin_path.is_file():
        print(f"Missing {bin_path}; build first.", file=sys.stderr)
        sys.exit(1)
    st = shutil.which("st-flash")
    if not st:
        print("st-flash not in PATH (install stlink tools).", file=sys.stderr)
        sys.exit(1)
    cmd = [st, "--connect-under-reset", "write", str(bin_path), FLASH_ADDR]
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def http_get(url: str, timeout: float = 3.0) -> str:
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode("utf-8", errors="replace")


def http_post_json(url: str, body: dict, timeout: float = 5.0) -> None:
    data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=timeout) as _:
        pass


def find_reachable_ip(candidates: list[str], timeout: float) -> str | None:
    for ip in candidates:
        try:
            r = http_get(f"http://{ip}/ping", timeout=timeout)
            if "pong" in r:
                print(f"OK: GET http://{ip}/ping -> pong")
                return ip
        except (urllib.error.URLError, OSError) as e:
            print(f"  skip {ip}: {e}")
    return None


def wait_uart_ready(port: str, baud: int, needle: str, overall_s: float) -> bool:
    try:
        import serial
    except ImportError:
        print("pyserial not installed; skip UART wait (pip install pyserial).", file=sys.stderr)
        return False
    deadline = time.time() + overall_s
    ser = serial.Serial(port, baud, timeout=0.5)
    ser.reset_input_buffer()
    buf = b""
    print(f"UART {port}: waiting for {needle!r} (up to {overall_s:.0f}s)...")
    while time.time() < deadline:
        buf += ser.read(512)
        if needle.encode() in buf or needle in buf.decode("utf-8", errors="replace"):
            ser.close()
            return True
    ser.close()
    return False


def poll_signed(base: str, timeout_s: float) -> dict | None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            raw = http_get(f"{base}/tx/signed", timeout=2.0)
            j = json.loads(raw)
            if j.get("status") == "signed" and j.get("sig"):
                return j
        except (urllib.error.URLError, OSError, json.JSONDecodeError):
            pass
        time.sleep(0.4)
    return None


def run_webusb_sign(scripts_dir: Path) -> int:
    py = sys.executable
    sign_py = scripts_dir / "test_usb_sign.py"
    if not sign_py.is_file():
        print(f"Missing {sign_py}", file=sys.stderr)
        return 1
    print("+", py, str(sign_py), "ping")
    r = subprocess.run([py, str(sign_py), "ping"])
    if r.returncode != 0:
        return r.returncode
    print("\n>>> Short-press USER on the board to confirm signing <<<\n")
    r = subprocess.run([py, str(sign_py), "sign"])
    return r.returncode


def audit_elf(elf: Path) -> int:
    """Warn if BIP39 test words appear in ELF (should not ship in production)."""
    if not elf.is_file():
        print(f"No ELF at {elf}", file=sys.stderr)
        return 1
    # strings via subprocess (avoid parsing binary in Python)
    strings = shutil.which("strings")
    if not strings:
        print("``strings`` not found; skip ELF audit.", file=sys.stderr)
        return 0
    p = subprocess.run(
        [strings, str(elf)],
        capture_output=True,
        text=True,
        errors="replace",
        check=False,
    )
    blob = p.stdout
    issues = 0
    if "abandon abandon" in blob:
        print("AUDIT WARN: test mnemonic fragment 'abandon abandon' found in ELF strings.")
        print("  OK for USE_TEST_SEED dev builds; remove for production / protected images.")
        issues += 1
    if issues == 0:
        print("ELF audit: no obvious test mnemonic fragment in strings output.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Build/flash/boot/smoke-test CryptoWallet signing (HTTP or WebUSB)."
    )
    ap.add_argument("--root", type=Path, default=None, help="Project root (default: repo root)")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--no-flash", action="store_true")
    ap.add_argument("--no-sign", action="store_true", help="Skip signing flow (build/flash/audit only)")
    ap.add_argument("--webusb", action="store_true", help="Use scripts/test_usb_sign.py instead of HTTP")
    ap.add_argument("--ip", default=None, help="Device IPv4 (otherwise try common static IPs)")
    ap.add_argument("--uart-wait", metavar="PORT", help="Wait for boot string on UART (e.g. /dev/ttyACM0)")
    ap.add_argument("--uart-baud", type=int, default=115200)
    ap.add_argument("--boot-string", default="Net: HTTP ready", help="Substring to wait in UART")
    ap.add_argument("--boot-wait-s", type=float, default=90.0)
    ap.add_argument("--confirm-wait-s", type=float, default=120.0, help="HTTP: poll /tx/signed after POST")
    ap.add_argument("--elf-audit-only", action="store_true", help="Only run strings audit on build/*.elf")
    args = ap.parse_args()

    root = args.root or project_root()
    scripts_dir = root / "scripts"
    elf = root / "build" / "cryptowallet.elf"

    make_extra = []
    if args.webusb:
        make_extra.append("USE_WEBUSB=1")

    if args.elf_audit_only:
        return audit_elf(elf)

    if not args.no_build:
        run_make(root, make_extra)

    audit_elf(elf)

    if args.no_sign and args.no_flash:
        return 0

    if not args.no_flash:
        run_flash(root)

    if args.no_sign:
        return 0

    if args.webusb:
        return run_webusb_sign(scripts_dir)

    ips: list[str] = list(DEFAULT_IP_CANDIDATES) if not args.ip else [args.ip]

    if args.uart_wait:
        wait_uart_ready(args.uart_wait, args.uart_baud, args.boot_string, args.boot_wait_s)

    ip = args.ip
    if not ip:
        ip = find_reachable_ip(ips, timeout=2.0)
    else:
        try:
            r = http_get(f"http://{ip}/ping", timeout=3.0)
            if "pong" not in r:
                print(f"Unexpected /ping response: {r!r}", file=sys.stderr)
                return 1
            print(f"OK: http://{ip}/ping")
        except (urllib.error.URLError, OSError) as e:
            print(f"Cannot reach http://{ip}/ping: {e}", file=sys.stderr)
            return 1

    if not ip:
        print(
            "Device not found. Set --ip, plug Ethernet, or wait for DHCP/static fallback.\n"
            "You can use --uart-wait /dev/ttyACM0 to pause until firmware prints boot string.",
            file=sys.stderr,
        )
        return 1

    base = f"http://{ip}"
    tx = {
        "recipient": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
        "amount": "0.001",
        "currency": "BTC",
    }
    print("+ POST /tx", tx)
    try:
        http_post_json(f"{base}/tx", tx, timeout=5.0)
    except (urllib.error.URLError, OSError) as e:
        print(f"POST /tx failed: {e}", file=sys.stderr)
        return 1

    print(f"\n>>> Short-press USER (PC13) within ~{args.confirm_wait_s:.0f}s <<<\n")
    j = poll_signed(base, args.confirm_wait_s)
    if not j:
        print("Timeout: no signature on GET /tx/signed", file=sys.stderr)
        return 1

    sig = j.get("sig", "")
    if not re.fullmatch(r"[0-9A-Fa-f]{128}", sig):
        print(f"Bad sig format: {sig[:40]}...", file=sys.stderr)
        return 1

    print("PASS: signed", sig[:32] + "...")
    return 0


if __name__ == "__main__":
    sys.exit(main())
