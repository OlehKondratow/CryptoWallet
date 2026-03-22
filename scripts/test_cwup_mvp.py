#!/usr/bin/env python3
"""
HIL smoke test: CWUP-0.1 on USART3 (AT+PING, AT+BOOTCHAIN?, optional AT+FWINFO? + CRC, stress rounds).

Requires **minimal-lwip** firmware **without** USE_RNG_DUMP (CWUP shares UART with TRNG dump).

Environment (override CLI):
  CWUP_UART_PORT, CI_UART_PORT  — serial device (default /dev/ttyACM0)
  CWUP_UART_BAUD, CI_UART_BAUD
  CWUP_SKIP_NO_DEVICE — if 1 and port missing: exit 0 (SKIP)
  CWUP_EXPECT_WALLET_SEED, CWUP_EXPECT_CRYPTO_SIGN — AT+WALLET? checks

Examples:
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --stress-extra-rounds 100
  CWUP_SKIP_NO_DEVICE=1 python3 scripts/test_cwup_mvp.py
  python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --bin build/cryptowallet.bin
"""
from __future__ import annotations

import argparse
import glob
import os
import stat
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


def _env_int(name: str, default: int) -> int:
    v = os.environ.get(name, "").strip()
    if not v:
        return default
    try:
        return int(v, 10)
    except ValueError:
        return default


def _is_char_device(path: str) -> bool:
    """True if path exists and is a character device (real serial port), not a stray directory."""
    try:
        st = os.stat(path)
        return stat.S_ISCHR(st.st_mode)
    except OSError:
        return False


def _ttyacm_char_devices() -> list[str]:
    return sorted(p for p in glob.glob("/dev/ttyACM*") if _is_char_device(p))


def _resolve_stlink_port(fallback: str) -> str:
    """
    Prefer ST-Link **MCU UART** VCP, not the ST-Link V3 **VCP Ctrl** interface.

    ST-Link V3 often exposes two ttyACM: one is "ST-Link VCP Ctrl" (not bridged to target
    USART3) and one is the actual virtual COM to the MCU. We deprioritize descriptions
    containing "VCP Ctrl" / "Ctrl" when multiple 0483 ports exist.
    """
    try:
        from serial.tools import list_ports
    except ImportError:
        return fallback

    def _is_stlink_candidate(hw: str, desc: str) -> bool:
        hw_u = hw.upper()
        d = desc.upper()
        if "0483:" not in hw_u:
            return False
        return bool(
            "5740" in hw_u
            or "3752" in hw_u
            or "374E" in hw_u
            or "ST-LINK" in d
            or "STLINK" in d
            or "STM32" in d
            or "STMICROELECTRONICS" in d
        )

    def _is_vcp_ctrl(desc: str) -> bool:
        # e.g. "ST-LINK/V3 - ST-Link VCP Ctrl" — not the USART3 bridge
        compact = (desc or "").upper().replace(" ", "")
        return "VCPCTRL" in compact

    candidates: list[tuple[str, str, str]] = []
    for p in list_ports.comports():
        hw = p.hwid or ""
        desc = p.description or ""
        if not _is_stlink_candidate(hw, desc):
            continue
        candidates.append((p.device, desc, hw))

    if not candidates:
        return fallback

    # Prefer non-"VCP Ctrl" (target UART bridge)
    non_ctrl = [c for c in candidates if not _is_vcp_ctrl(c[1])]
    if non_ctrl:
        non_ctrl.sort(key=lambda x: x[0])
        return non_ctrl[0][0]

    # Second ST-Link V3 interface (MCU USART bridge) often shows up as desc/hwid "n/a" in
    # pyserial, so it never enters `candidates`. If the only known ST port is VCP Ctrl,
    # pick another /dev/ttyACM* (typically /dev/ttyACM1). Skip bogus paths (e.g. ttyACM1 as
    # a directory — seen on some hosts).
    acms = _ttyacm_char_devices()
    for dev, desc, _hw in candidates:
        if _is_vcp_ctrl(desc):
            others = [a for a in acms if a != dev]
            if others:
                return others[0]

    candidates.sort(key=lambda x: x[0])
    return candidates[0][0]


def _print_serial_diagnostics() -> None:
    try:
        from serial.tools import list_ports
    except ImportError:
        print("--- (install pyserial with list_ports for port scan) ---", file=sys.stderr)
        return
    print("--- serial ports (pyserial) ---", file=sys.stderr)
    for p in list_ports.comports():
        print(f"  {p.device}  {p.description!r}  {p.hwid}", file=sys.stderr)
    print(
        "Hints: ST-Link V3 may show two ports — avoid 'VCP Ctrl' for CWUP; use the other "
        "ttyACM or scripts/test_cwup_mvp.py --auto-port. "
        "act/Docker: pass the host device (e.g. -v /dev/ttyACM0:/dev/ttyACM0).",
        file=sys.stderr,
    )


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
    p.add_argument(
        "--auto-port",
        action="store_true",
        help="Pick ST-Link MCU UART VCP (skips ST-Link V3 'VCP Ctrl'); else use --port",
    )
    p.add_argument(
        "--expect-wallet-seed",
        type=int,
        default=None,
        metavar="0|1",
        help="Expected AT+WALLET? seed flag (default: CWUP_EXPECT_WALLET_SEED or 0)",
    )
    p.add_argument(
        "--expect-crypto-sign",
        type=int,
        default=None,
        metavar="0|1",
        help="Expected AT+WALLET? crypto_sign flag (default: CWUP_EXPECT_CRYPTO_SIGN or 0)",
    )
    p.add_argument(
        "--echo-token",
        default=os.environ.get("CWUP_ECHO_TOKEN", "autotest"),
        help="Payload for AT+ECHO= (default env CWUP_ECHO_TOKEN or autotest)",
    )
    args = p.parse_args()
    if args.expect_wallet_seed is None:
        args.expect_wallet_seed = _env_int("CWUP_EXPECT_WALLET_SEED", 0)
    if args.expect_crypto_sign is None:
        args.expect_crypto_sign = _env_int("CWUP_EXPECT_CRYPTO_SIGN", 0)
    skip_nd = args.skip_no_device or _skip_no_device()

    try:
        import serial
    except ImportError:
        print("ERROR: pip install pyserial", file=sys.stderr)
        return 2

    port_name = _resolve_stlink_port(args.port) if args.auto_port else args.port
    if args.auto_port and port_name != args.port:
        print(f"OK: --auto-port selected {port_name} (was {args.port})", flush=True)

    port_path = Path(port_name)
    if not port_path.exists():
        if skip_nd:
            print(f"SKIP: no device {port_name}", flush=True)
            return 0
        print(f"ERROR: no serial device {port_name}", file=sys.stderr)
        _print_serial_diagnostics()
        return 2

    try:
        ser = serial.Serial(
            str(port_name),
            args.baud,
            timeout=args.line_timeout,
            write_timeout=2.0,
            dsrdtr=False,
            rtscts=False,
        )
    except (OSError, serial.SerialException) as e:
        if skip_nd:
            print(f"SKIP: cannot open {port_name}: {e}", flush=True)
            return 0
        print(f"ERROR: open {port_name}: {e}", file=sys.stderr)
        _print_serial_diagnostics()
        return 2

    if not args.auto_port:
        try:
            from serial.tools import list_ports

            for p in list_ports.comports():
                if p.device != str(port_name):
                    continue
                compact = (p.description or "").upper().replace(" ", "")
                if "VCPCTRL" in compact:
                    print(
                        "WARNING: this port looks like ST-Link 'VCP Ctrl' (not USART3 to MCU). "
                        "Use --auto-port or try the other ttyACM (e.g. /dev/ttyACM1).",
                        file=sys.stderr,
                    )
                break
        except Exception:
            pass

    try:
        ser.reset_input_buffer()
        text = drain_for_substring(
            ser,
            "CW+READY",
            args.ready_timeout,
            progress_interval=5.0,
        )
        if "CW+READY" not in text:
            print(
                "FAIL: CW+READY not seen in time "
                f"(got {len(text)} chars). Is CWUP enabled (not USE_RNG_DUMP)? "
                f"Port={port_name!r} baud={args.baud}",
                file=sys.stderr,
            )
            if text:
                print("--- RX tail ---", file=sys.stderr)
                print(text[-2000:], file=sys.stderr)
            else:
                _print_serial_diagnostics()
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
