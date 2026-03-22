"""Host-side checks for scripts/fw_integrity_check.py (IEEE CRC32 = firmware fw_integrity)."""

from __future__ import annotations

import binascii

import pytest

from fw_integrity_check import crc32_ieee


def test_crc32_ieee_empty() -> None:
    assert crc32_ieee(b"") == (binascii.crc32(b"") & 0xFFFFFFFF)


def test_crc32_ieee_known_vector() -> None:
    # binascii.crc32 matches POSIX / zlib CRC-32 (IEEE 802.3 polynomial), same as host tool
    data = b"123456789"
    assert crc32_ieee(data) == 0xCBF43926


def test_crc32_ieee_matches_binascii() -> None:
    for blob in (b"\x00" * 4096, b"\xff" * 100, bytes(range(256))):
        assert crc32_ieee(blob) == (binascii.crc32(blob) & 0xFFFFFFFF)


@pytest.mark.parametrize(
    "path",
    [
        pytest.param("build/cryptowallet.bin", id="minimal-build"),
    ],
)
def test_crc32_against_built_bin_if_present(path: str) -> None:
    """If firmware was built, CRC matches binascii over full file (optional local check)."""
    from pathlib import Path

    p = Path(__file__).resolve().parents[2] / path
    if not p.is_file():
        pytest.skip(f"no file at {p} (run make minimal-lwip first)")
    data = p.read_bytes()
    assert crc32_ieee(data) == (binascii.crc32(data) & 0xFFFFFFFF)
