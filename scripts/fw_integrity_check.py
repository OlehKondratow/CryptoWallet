#!/usr/bin/env python3
"""
Compute IEEE CRC32 over a firmware .bin (same algorithm as Core/Src/fw_integrity.c).

Usage:
  python3 scripts/fw_integrity_check.py build/cryptowallet.bin
  python3 scripts/fw_integrity_check.py build/cryptowallet.bin --expect-crc 0xAABBCCDD

Exit code 0 if --expect-crc matches (when given), else 0 always for print-only.
"""
from __future__ import annotations

import argparse
import binascii
import sys
from pathlib import Path


def crc32_ieee(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


def main() -> int:
    p = argparse.ArgumentParser(description="CRC32 (IEEE) for FW image — compare with UART FWINFO line")
    p.add_argument("bin_file", type=Path, help="Path to cryptowallet.bin")
    p.add_argument(
        "--expect-crc",
        type=str,
        default="",
        help="Expected CRC32 as 0xXXXXXXXX (optional; exit 1 on mismatch)",
    )
    args = p.parse_args()
    path = args.bin_file
    if not path.is_file():
        print(f"ERROR: not a file: {path}", file=sys.stderr)
        return 2
    data = path.read_bytes()
    c = crc32_ieee(data)
    print(f"file={path.resolve()}")
    print(f"size={len(data)}")
    print(f"crc32=0x{c:08X}")
    if args.expect_crc.strip():
        exp = int(args.expect_crc.strip(), 0)
        if exp != c:
            print(f"FAIL: expected 0x{exp:08X}, got 0x{c:08X}", file=sys.stderr)
            return 1
        print("OK: CRC match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
