#!/usr/bin/env python3
"""
Run dieharder on a binary file of raw random bytes (generator 201 = file_input_raw).

Doc index: README.md (repo root). Workflow: docs_src/rng-entropy.md , scripts/capture_rng_uart.py .

Install (Debian/Ubuntu):
  sudo apt install dieharder

Usage (from project root)
-------------------------
  python3 scripts/run_dieharder.py --file rng.bin
  python3 scripts/run_dieharder.py --file rng.bin --list-tests
  python3 scripts/run_dieharder.py --file rng.bin --test 1 --subtest 0
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys


def main() -> None:
    p = argparse.ArgumentParser(description="Run dieharder on a raw RNG binary file")
    p.add_argument("--file", "-f", default="rng.bin", help="Input binary (raw uint8 stream)")
    p.add_argument(
        "--dieharder",
        default="dieharder",
        help="dieharder executable name or path",
    )
    p.add_argument(
        "--list-tests",
        action="store_true",
        help="Print dieharder test list (-l) and exit",
    )
    p.add_argument(
        "--test",
        "-d",
        type=int,
        default=None,
        help="Run single test number (-d), e.g. 1",
    )
    p.add_argument(
        "--subtest",
        "-p",
        type=int,
        default=None,
        help="Subtest index (-p), often 0",
    )
    p.add_argument(
        "--psamples",
        type=int,
        default=None,
        help="Override psamples (-s) for some tests",
    )
    args = p.parse_args()

    exe = shutil.which(args.dieharder) or (
        args.dieharder if os.path.isfile(args.dieharder) else None
    )
    if not exe:
        print(
            "dieharder not found. Install: sudo apt install dieharder\n"
            "Or set --dieharder /path/to/dieharder",
            file=sys.stderr,
        )
        sys.exit(1)

    if args.list_tests:
        subprocess.run([exe, "-l"], check=False)
        return

    path = os.path.abspath(args.file)
    if not os.path.isfile(path):
        print(f"File not found: {path}", file=sys.stderr)
        sys.exit(1)

    size = os.path.getsize(path)
    if size < 1_000_000:
        print(
            f"Warning: file is only {size} bytes; many tests expect tens of MB or more.",
            file=sys.stderr,
        )

    # -g 201: file_input_raw (unsigned chars from file)
    cmd = [exe, "-g", "201", "-f", path]
    if args.test is not None:
        cmd.extend(["-d", str(args.test)])
    if args.subtest is not None:
        cmd.extend(["-p", str(args.subtest)])
    if args.psamples is not None:
        cmd.extend(["-s", str(args.psamples)])
    else:
        if args.test is None:
            cmd.append("-a")  # all tests

    print("Running:", " ".join(cmd), flush=True)
    ret = subprocess.run(cmd)
    sys.exit(ret.returncode)


if __name__ == "__main__":
    main()
