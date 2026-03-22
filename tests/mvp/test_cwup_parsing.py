"""Pure-Python tests for CWUP line parsing (no UART)."""

from __future__ import annotations

import pytest

from mvp_cwup import (
    parse_bootchain_line,
    parse_echo_response,
    parse_fwinfo_crc,
    parse_selftest_line,
    parse_wallet_line,
)


@pytest.mark.parametrize(
    "line,expected",
    [
        ("CW+FWINFO,FWINFO crc32=0xDEADBEEF,len=1,start=0x0,end=0x1", 0xDEADBEEF),
        ("FWINFO crc32=0x00AABBCC,len=100,start=0x08000000,end=0x08020000", 0x00AABBCC),
        ("no crc here", None),
    ],
)
def test_parse_fwinfo_crc(line: str, expected: int | None) -> None:
    assert parse_fwinfo_crc(line) == expected


def test_parse_bootchain_line() -> None:
    line = (
        "CW+BOOTCHAIN,rom=STM32,verified_boot=0,"
        "app_entry=0x08010080,note=app+stm32_secure_boot"
    )
    d = parse_bootchain_line(line)
    assert d is not None
    assert d.get("verified_boot") == 0
    assert d.get("app_entry") == 0x08010080
    assert "stm32" in str(d.get("note", "")).lower() or "note" in d


def test_parse_wallet_line() -> None:
    line = "CW+WALLET,seed=1,crypto_sign=0"
    w = parse_wallet_line(line)
    assert w == {"seed": 1, "crypto_sign": 0}


def test_parse_selftest_line() -> None:
    assert parse_selftest_line("CW+SELFTEST,ok=1,tick=12345") is True
    assert parse_selftest_line("CW+SELFTEST,ok=0") is False


def test_parse_echo_response() -> None:
    assert parse_echo_response("CW+ECHO,hello") == "hello"
    assert parse_echo_response("CW+ECHO,") == ""
