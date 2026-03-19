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

Usage (from project root)
-------------------------
  pip install -r scripts/requirements.txt
  python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728

Recommended file size for dieharder -a: >= 128 MiB (134217728 bytes); more is better.
"""

from __future__ import annotations

import argparse
import sys
import time


def capture(port: str, baud: int, out_path: str, total_bytes: int, skip_bytes: int) -> int:
    try:
        import serial
    except ImportError:
        print("Install pyserial: pip install -r scripts/requirements.txt", file=sys.stderr)
        sys.exit(1)

    ser = serial.Serial(port=port, baudrate=baud, timeout=1.0)
    time.sleep(0.2)
    ser.reset_input_buffer()

    written = 0
    skip_left = skip_bytes

    with open(out_path, "wb") as f:
        while written < total_bytes:
            chunk = ser.read(min(4096, total_bytes - written + skip_left))
            if not chunk:
                continue
            if skip_left > 0:
                if len(chunk) <= skip_left:
                    skip_left -= len(chunk)
                    continue
                chunk = chunk[skip_left:]
                skip_left = 0
            take = min(len(chunk), total_bytes - written)
            f.write(chunk[:take])
            written += take
            if written % (8 * 1024 * 1024) == 0 and written > 0:
                print(f"  {written / (1024 * 1024):.0f} MiB", flush=True)

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
    args = p.parse_args()

    if args.bytes < 1_000_000:
        print("Warning: < 1 MiB may be too small for full dieharder -a reliability.", file=sys.stderr)

    print(f"Capturing {args.bytes} bytes from {args.port} @ {args.baud} -> {args.out}")
    n = capture(args.port, args.baud, args.out, args.bytes, args.skip)
    print(f"Done: {n} bytes written.")


if __name__ == "__main__":
    main()
