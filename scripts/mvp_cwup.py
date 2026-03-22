"""
Helpers for CWUP MVP UART tests (line I/O, FWINFO parsing).

Used by scripts/test_cwup_mvp.py and tests/mvp/test_cwup_parsing.py.
"""
from __future__ import annotations

import re
import sys
import time
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    import serial


_FWINFO_CRC_RE = re.compile(r"crc32\s*=\s*(0x[0-9A-Fa-f]{8}|[0-9]+)", re.I)
_WALLET_RE = re.compile(r"CW\+WALLET,seed\s*=\s*([01]),crypto_sign\s*=\s*([01])", re.I)
_SELFTEST_OK_RE = re.compile(r"CW\+SELFTEST,ok\s*=\s*1(?:,tick\s*=\s*(\d+))?", re.I)
_CW_READY_LINE_RE = re.compile(r"CW\+READY\s*,", re.I)
_BOOTCHAIN_ENTRY_RE = re.compile(r"app_entry\s*=\s*(0x[0-9A-Fa-f]{8,16})", re.I)
_BOOTCHAIN_VB_RE = re.compile(r"verified_boot\s*=\s*([01])", re.I)


def parse_bootchain_line(line: str) -> dict[str, int | str] | None:
    """
    Parse CW+BOOTCHAIN,... line from AT+BOOTCHAIN?.

    Returns dict with keys: rom, verified_boot (0/1), app_entry (int), note (str) if parseable.
    """
    s = line.strip()
    if "CW+BOOTCHAIN" not in s:
        return None
    out: dict[str, int | str] = {}
    if "rom=" in s:
        m = re.search(r"rom\s*=\s*([^,]+)", s)
        if m:
            out["rom"] = m.group(1).strip()
    mvb = _BOOTCHAIN_VB_RE.search(s)
    if mvb:
        out["verified_boot"] = int(mvb.group(1), 10)
    ment = _BOOTCHAIN_ENTRY_RE.search(s)
    if ment:
        out["app_entry"] = int(ment.group(1), 0)
    mnote = re.search(r"note\s*=\s*(.+)$", s)
    if mnote:
        out["note"] = mnote.group(1).strip()
    return out if out else None


def parse_wallet_line(line: str) -> dict[str, int] | None:
    """Parse CW+WALLET,seed=0|1,crypto_sign=0|1."""
    m = _WALLET_RE.search(line)
    if not m:
        return None
    return {"seed": int(m.group(1), 10), "crypto_sign": int(m.group(2), 10)}


def parse_selftest_line(line: str) -> bool:
    """True if CW+SELFTEST reports ok=1."""
    return _SELFTEST_OK_RE.search(line) is not None


def parse_echo_response(line: str) -> str | None:
    """Payload after first comma in CW+ECHO,..."""
    s = line.strip()
    if not s.upper().startswith("CW+ECHO"):
        return None
    idx = s.find(",")
    if idx < 0:
        return ""
    return s[idx + 1 :]


def parse_fwinfo_crc(line: str) -> int | None:
    """
    Extract CRC32 from a CW+FWINFO,... line or inner FWINFO payload.

    Example payload: FWINFO crc32=0xA1B2C3D4,len=...,start=...,end=...
    """
    m = _FWINFO_CRC_RE.search(line)
    if not m:
        return None
    return int(m.group(1), 0)


def first_cw_ready_line(rx_text: str) -> str | None:
    """
    First line containing ``CW+READY,...`` (CWUP phase B marker).

    Lines before it are phase A (optional TEXT preamble / logs). Used by CI/tests to
    assert the host does not treat the stream as CWUP-ready until this line appears.
    """
    if not rx_text:
        return None
    for line in rx_text.splitlines():
        if _CW_READY_LINE_RE.search(line):
            return line.strip()
    return None


def phase_a_prefix_before_ready(rx_text: str) -> str:
    """
    Text before the first line that matches ``CW+READY,...`` (phase A only).

    If ``CW+READY`` is absent, returns ``rx_text`` unchanged (e.g. timeout drain).
    """
    if not rx_text:
        return ""
    out: list[str] = []
    for line in rx_text.splitlines(keepends=True):
        if _CW_READY_LINE_RE.search(line):
            return "".join(out)
        out.append(line)
    return rx_text


def drain_for_substring(
    ser: "serial.Serial",
    needle: str,
    total_timeout: float,
    *,
    progress_interval: float | None = None,
) -> str:
    """
    Accumulate decoded text until `needle` appears or timeout.

    If ``progress_interval`` is set (e.g. 5.0), print a status line to stderr every N
    seconds (helps when ST-Link V3 wrong tty or no firmware output).
    """
    deadline = time.monotonic() + total_timeout
    acc = ""
    last_prog = time.monotonic()
    while time.monotonic() < deadline:
        if ser.in_waiting:
            acc += ser.read(ser.in_waiting).decode("utf-8", errors="replace")
            if needle in acc:
                return acc
        else:
            time.sleep(0.02)
        if progress_interval is not None and progress_interval > 0:
            now = time.monotonic()
            if now - last_prog >= progress_interval:
                last_prog = now
                left = max(0.0, deadline - now)
                print(
                    f"... waiting for {needle!r} ({len(acc)} bytes rx, ~{left:.0f}s left)",
                    flush=True,
                    file=sys.stderr,
                )
    return acc


def send_command(ser: "serial.Serial", cmd: str) -> None:
    line = cmd if cmd.endswith("\r\n") else cmd + "\r\n"
    ser.write(line.encode("ascii", errors="strict"))
    ser.flush()


def read_response_line(ser: "serial.Serial") -> str:
    """One line from UART (timeout = ser.timeout)."""
    raw = ser.readline()
    return raw.decode("utf-8", errors="replace").strip()
