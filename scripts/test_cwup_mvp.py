#!/usr/bin/env python3
"""
HIL smoke test: CWUP-0.1 on USART3 (AT+PING, AT+BOOTCHAIN?, optional AT+FWINFO? + CRC, stress rounds).

Requires **minimal-lwip** firmware **without** USE_RNG_DUMP (CWUP shares UART with TRNG dump).

Environment (override CLI):
  CWUP_UART_PORT, CI_UART_PORT  — serial device (default /dev/ttyACM0)
  CWUP_UART_BAUD, CI_UART_BAUD
  CWUP_SKIP_NO_DEVICE — if 1 and port missing: exit 0 (SKIP)

Examples:
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --stress-extra-rounds 100
  CWUP_SKIP_NO_DEVICE=1 python3 scripts/test_cwup_mvp.py
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --bin build/cryptowallet.bin
"""
from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path

# Allow running as script: repo_root/scripts/
_REPO = Path(__file__).resolve().parents[1]
if str(_REPO / "scripts") not in sys.path:
    sys.path.insert(0, str(_REPO / "scripts"))

from fw_integrity_check import crc32_ieee  # noqa: E402
from mvp_cwup import (  # noqa: E402
    drain_for_substring,
    parse_bootchain_line,
    parse_echo_response,
    parse_fwinfo_crc,
    parse_selftest_line,
    parse_wallet_line,
    read_response_line,
    send_command,
)


def _port_default() -> str:
    return os.environ.get("CWUP_UART_PORT") or os.environ.get("CI_UART_PORT") or "/dev/ttyACM0"


def _baud_default() -> int:
    raw = os.environ.get("CWUP_UART_BAUD") or os.environ.get("CI_UART_BAUD") or "115200"
    return int(raw, 10)


def _skip_no_device() -> bool:
    v = os.environ.get("CWUP_SKIP_NO_DEVICE", "")
    return v.lower() in ("1", "true", "yes")


def run_command_round(
    ser,
    *,
    bin_path: Path | None,
    round_label: str,
    expect_wallet_seed: int = 0,
    expect_crypto_sign: int = 0,
    echo_token: str = "autotest",
) -> int:
    """
    One full CWUP command sequence (after CW+READY already seen).
    Lab/CI: AT+WALLET? / AT+SELFTEST? / AT+ECHO= (no authentication).
    Returns 0 on success.
    """
    basic: list[tuple[str, str, str]] = [
        ("AT+PING", "CW+PONG", "PING"),
        ("AT+CWINFO?", "CW+CWINFO", "CWINFO"),
        ("AT+READY?", "CW+READY", "READY"),
    ]

    for cmd, need, tag in basic:
        send_command(ser, cmd)
        line = read_response_line(ser)
        if need not in line:
            print(
                f"FAIL [{round_label}] {tag}: expected substring {need!r}, got {line!r}",
                file=sys.stderr,
            )
            return 1
        print(f"OK [{round_label}] {tag}: {line}", flush=True)

    send_command(ser, "AT+WALLET?")
    line = read_response_line(ser)
    if "CW+WALLET" not in line:
        print(f"FAIL [{round_label}] WALLET: {line!r}", file=sys.stderr)
        return 1
    w = parse_wallet_line(line)
    if w is None:
        print(f"FAIL [{round_label}] WALLET parse: {line!r}", file=sys.stderr)
        return 1
    if w["seed"] != expect_wallet_seed or w["crypto_sign"] != expect_crypto_sign:
        print(
            f"FAIL [{round_label}] WALLET want seed={expect_wallet_seed} "
            f"crypto_sign={expect_crypto_sign}, got {w}",
            file=sys.stderr,
        )
        return 1
    print(f"OK [{round_label}] WALLET: {line}", flush=True)

    send_command(ser, "AT+SELFTEST?")
    line = read_response_line(ser)
    if not parse_selftest_line(line):
        print(f"FAIL [{round_label}] SELFTEST: {line!r}", file=sys.stderr)
        return 1
    print(f"OK [{round_label}] SELFTEST: {line}", flush=True)

    echo_cmd = "AT+ECHO=" + echo_token
    send_command(ser, echo_cmd)
    line = read_response_line(ser)
    echoed = parse_echo_response(line)
    if echoed != echo_token:
        print(
            f"FAIL [{round_label}] ECHO want payload {echo_token!r}, got {line!r}",
            file=sys.stderr,
        )
        return 1
    print(f"OK [{round_label}] ECHO: {line}", flush=True)

    if bin_path is not None:
        send_command(ser, "AT+FWINFO?")
        line = read_response_line(ser)
        if "CW+FWINFO" not in line:
            print(f"FAIL [{round_label}] FWINFO: {line!r}", file=sys.stderr)
            return 1
        crc_dev = parse_fwinfo_crc(line)
        if crc_dev is None:
            print(f"FAIL [{round_label}] parse crc32 from {line!r}", file=sys.stderr)
            return 1
        if not bin_path.is_file():
            print(f"ERROR: --bin not a file: {bin_path}", file=sys.stderr)
            return 2
        crc_host = crc32_ieee(bin_path.read_bytes())
        if crc_dev != crc_host:
            print(
                f"FAIL [{round_label}] FWINFO crc32=0x{crc_dev:08X} != host 0x{crc_host:08X}",
                file=sys.stderr,
            )
            return 1
        print(f"OK [{round_label}] FWINFO crc matches file (0x{crc_host:08X})", flush=True)

    send_command(ser, "AT+BOOTCHAIN?")
    line = read_response_line(ser)
    if "CW+BOOTCHAIN" not in line:
        print(f"FAIL [{round_label}] BOOTCHAIN: {line!r}", file=sys.stderr)
        return 1
    info = parse_bootchain_line(line)
    if not info or "app_entry" not in info:
        print(f"FAIL [{round_label}] BOOTCHAIN parse: {line!r}", file=sys.stderr)
        return 1
    print(f"OK [{round_label}] BOOTCHAIN: {line}", flush=True)
    return 0


def main() -> int:
    p = argparse.ArgumentParser(description="CWUP MVP UART smoke test + stress rounds")
    p.add_argument("--port", default=_port_default(), help="Serial device")
    p.add_argument("--baud", type=int, default=_baud_default())
    p.add_argument(
        "--ready-timeout",
        type=float,
        default=30.0,
        help="Seconds to wait for CW+READY substring after open",
    )
    p.add_argument("--line-timeout", type=float, default=3.0, help="Per readline timeout (s)")
    p.add_argument(
        "--bin",
        type=Path,
        default=None,
        help="If set: AT+FWINFO? CRC must match CRC32 of this file (same as flashed image)",
    )
    p.add_argument(
        "--stress-extra-rounds",
        type=int,
        default=0,
        metavar="N",
        help="After first successful pass, repeat the full command sequence N more times",
    )
    p.add_argument(
        "--stress-delay-ms",
        type=int,
        default=0,
        help="Pause between stress rounds (ms)",
    )
    p.add_argument(
        "--skip-no-device",
        action="store_true",
        help="Exit 0 if serial port does not exist (also CWUP_SKIP_NO_DEVICE=1)",
    )
    args = p.parse_args()
    skip_nd = args.skip_no_device or _skip_no_device()

    try:
        import serial
    except ImportError:
        print("ERROR: pip install pyserial", file=sys.stderr)
        return 2

    port_path = Path(args.port)
    if not port_path.exists():
        if skip_nd:
            print(f"SKIP: no device {args.port}", flush=True)
            return 0
        print(f"ERROR: no serial device {args.port}", file=sys.stderr)
        return 2

    try:
        ser = serial.Serial(str(args.port), args.baud, timeout=args.line_timeout)
    except (OSError, serial.SerialException) as e:
        if skip_nd:
            print(f"SKIP: cannot open {args.port}: {e}", flush=True)
            return 0
        print(f"ERROR: open {args.port}: {e}", file=sys.stderr)
        return 2

    try:
        ser.reset_input_buffer()
        text = drain_for_substring(ser, "CW+READY", args.ready_timeout)
        if "CW+READY" not in text:
            print(
                "FAIL: CW+READY not seen in time "
                f"(got {len(text)} chars). Is CWUP enabled (not USE_RNG_DUMP)?",
                file=sys.stderr,
            )
            if text:
                print("--- RX tail ---", file=sys.stderr)
                print(text[-2000:], file=sys.stderr)
            return 1
        print("OK: saw CW+READY (banner)", flush=True)

        rc = run_command_round(
            ser,
            bin_path=args.bin,
            round_label="1",
            expect_wallet_seed=args.expect_wallet_seed,
            expect_crypto_sign=args.expect_crypto_sign,
            echo_token=args.echo_token,
        )
        if rc != 0:
            return rc

        total_rounds = 1 + max(0, args.stress_extra_rounds)
        for i in range(2, total_rounds + 1):
            if args.stress_delay_ms > 0:
                time.sleep(args.stress_delay_ms / 1000.0)
            rc = run_command_round(
                ser,
                bin_path=args.bin,
                round_label=str(i),
                expect_wallet_seed=args.expect_wallet_seed,
                expect_crypto_sign=args.expect_crypto_sign,
                echo_token=args.echo_token,
            )
            if rc != 0:
                return rc

        print(
            f"CWUP MVP: all checks passed ({total_rounds} round(s), "
            f"stress_extra={args.stress_extra_rounds})",
            flush=True,
        )
        return 0
    finally:
        try:
            ser.close()
        except OSError:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
