"""Host tests for scripts/fw_integrity_check.py (Block 1: .bin vs FWINFO line)."""

from __future__ import annotations

import binascii

from fw_integrity_check import (
    compare_bin_to_fwinfo_line,
    compare_bin_to_fwinfo_payload,
    crc32_ieee,
    parse_fwinfo_payload,
)


def test_crc32_ieee_known() -> None:
    data = b"123456789"
    assert crc32_ieee(data) == (binascii.crc32(data) & 0xFFFFFFFF)


def test_parse_fwinfo_payload_from_cw_line() -> None:
    line = (
        "CW+FWINFO,FWINFO crc32=0xAABBCCDD,len=100,start=0x08000000,end=0x08000064"
    )
    d = parse_fwinfo_payload(line)
    assert d is not None
    assert d["crc32"] == 0xAABBCCDD
    assert d["len"] == 100
    assert d["start"] == 0x08000000
    assert d["end"] == 0x08000064


def test_parse_fwinfo_payload_log_only() -> None:
    line = "FWINFO crc32=99,len=4,start=0x1,end=0x5"
    d = parse_fwinfo_payload(line)
    assert d is not None
    assert d["crc32"] == 99
    assert d["len"] == 4


def test_parse_fwinfo_payload_invalid() -> None:
    assert parse_fwinfo_payload("no payload here") is None


def test_compare_bin_match() -> None:
    data = b"hello"
    c = crc32_ieee(data)
    line = (
        f"FWINFO crc32=0x{c:08X},len={len(data)},start=0x08000000,end=0x08000005"
    )
    ok, msg = compare_bin_to_fwinfo_line(data, line)
    assert ok, msg


def test_compare_bin_crc_mismatch() -> None:
    data = b"x"
    line = "FWINFO crc32=0x00000000,len=1,start=0x0,end=0x1"
    ok, msg = compare_bin_to_fwinfo_line(data, line)
    assert not ok
    assert "CRC mismatch" in msg


def test_compare_bin_len_mismatch() -> None:
    data = b"ab"
    c = crc32_ieee(data)
    line = f"FWINFO crc32=0x{c:08X},len=1,start=0x0,end=0x2"
    ok, msg = compare_bin_to_fwinfo_line(data, line)
    assert not ok
    assert "Length mismatch" in msg


def test_compare_bin_to_fwinfo_payload_direct() -> None:
    data = b"\x00\x01\x02"
    c = crc32_ieee(data)
    fw = {"crc32": c, "len": 3, "start": 0, "end": 3}
    ok, msg = compare_bin_to_fwinfo_payload(data, fw)
    assert ok, msg
