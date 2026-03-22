#!/usr/bin/env python3
"""
Compute IEEE CRC32 over a firmware .bin (same polynomial as Core/Src/fw_integrity.c).

The device computes CRC over the contiguous Flash range
``[__app_flash_start, __app_flash_end)`` (see ``fw_integrity_init()``). For the printed
``crc32`` / ``size`` to match ``AT+FWINFO?`` or the ``FWINFO`` log line, the file must be
the same byte image as that range (same length and content). Typical workflow: build
``build/cryptowallet.bin`` with the same linker script and ``objcopy`` layout as flashed;
if length or padding differs from ``len`` on the device, CRCs will not match even when the
app logic is correct.

Parse a device line with ``parse_fwinfo_payload()`` (substring ``FWINFO crc32=...,len=...``)
and compare a file with ``compare_bin_to_fwinfo_line()`` — used by tests and optional CLI.

Usage:
  python3 scripts/fw_integrity_check.py build/cryptowallet.bin
  python3 scripts/fw_integrity_check.py build/cryptowallet.bin --expect-crc 0xAABBCCDD
  python3 scripts/fw_integrity_check.py build/cryptowallet.bin \\
    --expect-fwinfo-line 'CW+FWINFO,FWINFO crc32=0x...,len=...,start=0x...,end=0x...'

Exit code 0 if checks pass; 1 on mismatch (when expectations given); 2 on I/O or parse errors.
"""
from __future__ import annotations

import argparse
import binascii
import re
import sys
from pathlib import Path

# Same shape as fw_integrity_snprint() in Core/Src/fw_integrity.c
_FWINFO_PAYLOAD_RE = re.compile(
    r"FWINFO\s+crc32\s*=\s*(0x[0-9A-Fa-f]{8}|[0-9]+)\s*,\s*"
    r"len\s*=\s*([0-9]+)\s*,\s*"
    r"start\s*=\s*(0x[0-9A-Fa-f]+)\s*,\s*"
    r"end\s*=\s*(0x[0-9A-Fa-f]+)",
    re.I,
)


def crc32_ieee(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


def parse_fwinfo_payload(text: str) -> dict[str, int] | None:
    """
    Extract crc32, len, start, end from a log or CWUP line containing the FWINFO payload.

    Example (from device): ``FWINFO crc32=0x12345678,len=65536,start=0x08000000,end=0x08010000``
    """
    m = _FWINFO_PAYLOAD_RE.search(text)
    if not m:
        return None
    return {
        "crc32": int(m.group(1), 0),
        "len": int(m.group(2), 10),
        "start": int(m.group(3), 0),
        "end": int(m.group(4), 0),
    }


def compare_bin_to_fwinfo_payload(data: bytes, fw: dict[str, int]) -> tuple[bool, str]:
    """Return (ok, message). Checks CRC32 and file length vs FWINFO len."""
    c = crc32_ieee(data)
    if c != fw["crc32"]:
        return False, f"CRC mismatch: file 0x{c:08X} vs FWINFO 0x{fw['crc32']:08X}"
    if len(data) != fw["len"]:
        return (
            False,
            f"Length mismatch: file {len(data)} bytes vs FWINFO len={fw['len']}",
        )
    return True, "OK: CRC and length match FWINFO"


def compare_bin_to_fwinfo_line(data: bytes, line: str) -> tuple[bool, str]:
    fw = parse_fwinfo_payload(line)
    if fw is None:
        return False, "Could not parse FWINFO crc32=...,len=...,start=...,end=... from line"
    return compare_bin_to_fwinfo_payload(data, fw)


def main() -> int:
    p = argparse.ArgumentParser(description="CRC32 (IEEE) for FW image — compare with UART FWINFO line")
    p.add_argument("bin_file", type=Path, help="Path to cryptowallet.bin")
    p.add_argument(
        "--expect-crc",
        type=str,
        default="",
        help="Expected CRC32 as 0xXXXXXXXX (optional; exit 1 on mismatch)",
    )
    p.add_argument(
        "--expect-fwinfo-line",
        type=str,
        default="",
        metavar="LINE",
        help="Full UART/CWUP line containing FWINFO crc32=...,len=...,start=...,end=... (CRC + length check)",
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

    if args.expect_fwinfo_line.strip():
        ok, msg = compare_bin_to_fwinfo_line(data, args.expect_fwinfo_line)
        print(msg)
        if not ok:
            return 1
    elif args.expect_crc.strip():
        exp = int(args.expect_crc.strip(), 0)
        if exp != c:
            print(f"FAIL: expected 0x{exp:08X}, got 0x{c:08X}", file=sys.stderr)
            return 1
        print("OK: CRC match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
