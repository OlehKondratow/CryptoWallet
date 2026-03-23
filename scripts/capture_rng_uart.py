#!/usr/bin/env python3
"""
Capture raw random bytes from the device UART into a binary file for DIEHARDER.

Doc index: README.md (repo root). RNG background: docs_src/rng-entropy.md .

Prerequisites on the device
---------------------------
Firmware must stream **only raw bytes** (no UART_Log / printf text). Typical options:

1. Add a diagnostic mode (e.g. USE_RNG_DUMP=1) that calls random_buffer() / HAL_RNG
   in a loop and writes bytes to the same UART used for logs — **without** any text.
2. Or use a dedicated USB CDC endpoint that sends binary only.

If ASCII logs are mixed into the stream, statistical tests are invalid.

Environment (defaults for CI)
-------------------------------
  CI_RNG_FIRST_BYTE_TIMEOUT_SEC — abort if no byte received (default 90)
  CI_RNG_STALL_TIMEOUT_SEC      — abort if no progress for N seconds (default 180)
  CI_RNG_PROGRESS_INTERVAL_SEC  — print progress at least every N s (default 5)

Usage (from project root)
-------------------------
  pip install -r scripts/requirements.txt
  python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728

Recommended file size for dieharder -a: >= 128 MiB (134217728 bytes); more is better.
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path


def _env_float(name: str, default: float) -> float:
    v = os.environ.get(name, "").strip()
    if not v:
        return default
    try:
        return float(v)
    except ValueError:
        return default


def _env_int(name: str, default: int) -> int:
    v = os.environ.get(name, "").strip()
    if not v:
        return default
    try:
        return int(v)
    except ValueError:
        return default


def capture(
    port: str,
    baud: int,
    out_path: str,
    total_bytes: int,
    skip_bytes: int,
    *,
    first_byte_timeout: float,
    stall_timeout: float,
    progress_interval_sec: float,
    progress_every_bytes: int,
) -> int:
    try:
        import serial
        from serial.serialutil import SerialException
    except ImportError:
        print("Install pyserial: pip install -r scripts/requirements.txt", file=sys.stderr)
        sys.exit(1)

    out_p = Path(out_path).resolve()
    out_p.parent.mkdir(parents=True, exist_ok=True)
    # Файл появляется сразу (0 байт), чтобы в CI/артефакте был виден результат даже при ошибке
    out_p.write_bytes(b"")
    print(f"📁 Output file created (empty): {out_p}", flush=True)

    try:
        ser = serial.Serial(port=port, baudrate=baud, timeout=1.0)
    except OSError as e:
        print(f"::error::Cannot open {port}: {e}", flush=True)
        print(
            "Подсказка: нет устройства, занят другим процессом или нет прав (dialout).",
            flush=True,
        )
        raise SystemExit(1) from e

    time.sleep(0.2)
    try:
        ser.reset_input_buffer()
    except OSError:
        pass

    written = 0
    skip_left = skip_bytes
    t_start = time.monotonic()
    t_last_progress = written  # bytes count at last progress line
    t_last_progress_time = t_start
    t_last_data_time = t_start  # last time we got UART data (после skip — рост written)
    saw_uart_rx = False  # любой непустой chunk с порта (учёт --skip)

    print(
        f"Capturing {total_bytes} bytes from {port} @ {baud} -> {out_p}\n"
        f"   (first-byte timeout {first_byte_timeout}s, stall timeout {stall_timeout}s)\n"
        f"   Progress every {progress_every_bytes} bytes or {progress_interval_sec}s",
        flush=True,
    )

    with open(out_p, "wb") as f:
        while written < total_bytes:
            now = time.monotonic()

            if not saw_uart_rx and now - t_start >= first_byte_timeout:
                msg = (
                    f"No UART data for {first_byte_timeout:.0f}s — aborting.\n"
                    "Ожидается бинарный поток RNG (прошивка с USE_RNG_DUMP=1).\n"
                    "Если CI собран с CI_BUILD_USE_RNG_DUMP=0, на UART идут только текстовые логи."
                )
                print(f"::error::{msg}", flush=True)
                print(msg, flush=True)
                f.flush()
                raise SystemExit(2)

            if written > 0 and now - t_last_data_time >= stall_timeout:
                msg = (
                    f"No progress for {stall_timeout:.0f}s (stuck at {written} bytes). Aborting."
                )
                print(f"::error::{msg}", flush=True)
                print(msg, flush=True)
                f.flush()
                raise SystemExit(3)

            try:
                chunk = ser.read(min(4096, total_bytes - written + skip_left))
            except SerialException as e:
                # e.g. "device disconnected or multiple access on port" (minicom, another reader)
                print(
                    f"::warning::UART read error (will retry): {e}\n"
                    "    Закройте minicom/другие читатели порта; один процесс на tty.",
                    flush=True,
                )
                time.sleep(0.25)
                continue
            if not chunk:
                continue

            saw_uart_rx = True
            t_last_data_time = now

            if skip_left > 0:
                if len(chunk) <= skip_left:
                    skip_left -= len(chunk)
                    continue
                chunk = chunk[skip_left:]
                skip_left = 0

            take = min(len(chunk), total_bytes - written)
            f.write(chunk[:take])
            f.flush()
            written += take

            now = time.monotonic()
            should_print = (
                written - t_last_progress >= progress_every_bytes
                or now - t_last_progress_time >= progress_interval_sec
                or written >= total_bytes
            )
            if should_print and written > 0:
                elapsed = now - t_start
                pct = 100.0 * written / total_bytes
                rate = written / elapsed if elapsed > 0 else 0.0
                remain = total_bytes - written
                eta = remain / rate if rate > 0 else float("inf")
                eta_s = f"{eta:.0f}s" if eta < 86400 else "∞"
                print(
                    f"  Progress: {written}/{total_bytes} bytes ({pct:.1f}%) | "
                    f"{rate / 1024:.2f} KiB/s | elapsed {elapsed:.1f}s | ETA ~{eta_s}",
                    flush=True,
                )
                t_last_progress = written
                t_last_progress_time = now

    ser.close()
    return written


def main() -> None:
    p = argparse.ArgumentParser(description="Capture raw RNG bytes from UART for DIEHARDER")
    p.add_argument("--port", default="/dev/ttyACM0", help="Serial device (e.g. COM3, /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=115200, help="UART baud rate")
    p.add_argument("--out", default="rng.bin", help="Output binary file")
    p.add_argument(
        "--bytes",
        type=int,
        default=134_217_728,
        help="Total random bytes to save (default: 128 MiB)",
    )
    p.add_argument(
        "--skip",
        type=int,
        default=0,
        help="Skip first N bytes (e.g. discard boot garbage before stream starts)",
    )
    p.add_argument(
        "--first-byte-timeout",
        type=float,
        default=_env_float("CI_RNG_FIRST_BYTE_TIMEOUT_SEC", 90.0),
        help="Abort if no byte received within this many seconds (env: CI_RNG_FIRST_BYTE_TIMEOUT_SEC)",
    )
    p.add_argument(
        "--stall-timeout",
        type=float,
        default=_env_float("CI_RNG_STALL_TIMEOUT_SEC", 180.0),
        help="Abort if no new bytes for this many seconds (env: CI_RNG_STALL_TIMEOUT_SEC)",
    )
    p.add_argument(
        "--progress-interval",
        type=float,
        default=_env_float("CI_RNG_PROGRESS_INTERVAL_SEC", 5.0),
        help="Print progress at least every N seconds",
    )
    p.add_argument(
        "--progress-bytes",
        type=int,
        default=_env_int("CI_RNG_PROGRESS_EVERY_BYTES", 64 * 1024),
        help="Print progress every N bytes (whichever comes first with interval)",
    )
    args = p.parse_args()

    if args.bytes < 1_000_000:
        print(
            "Warning: < 1 MiB may be too small for full dieharder -a reliability.",
            file=sys.stderr,
        )

    try:
        n = capture(
            args.port,
            args.baud,
            args.out,
            args.bytes,
            args.skip,
            first_byte_timeout=args.first_byte_timeout,
            stall_timeout=args.stall_timeout,
            progress_interval_sec=args.progress_interval,
            progress_every_bytes=args.progress_bytes,
        )
    except SystemExit:
        raise
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        raise SystemExit(130) from None

    print(f"Done: {n} bytes written -> {Path(args.out).resolve()}", flush=True)


if __name__ == "__main__":
    main()
