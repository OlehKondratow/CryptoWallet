#!/usr/bin/env python3
"""
Читает UART до появления ожидаемых подстрок (boot / LwIP / задачи / сеть / SNTP).

По умолчанию порядок строк **не важен**: успех, когда **все** подстроки из файла
маркеров хотя бы раз встретились в накопленном логе (типичная успешная загрузка).

Режим строгого порядка: --ordered или CI_UART_MARKERS_ORDERED=1 .

Использование в CI (корень репозитория):
  pip install pyserial
  python3 scripts/ci/uart_wait_boot_log.py

Переменные окружения (перекрывают значения по умолчанию):
  CI_UART_PORT              — устройство (по умолчанию /dev/ttyACM0)
  CI_UART_BAUD              — 115200
  CI_UART_BOOT_TIMEOUT_SEC  — общий таймаут (по умолчанию 120)
  CI_UART_MARKERS_FILE      — файл маркеров
  CI_UART_LOG_OUT           — полный лог (uart_output.log)
  CI_UART_STATUS_OUT        — файл статуса (uart_boot_status.txt)
  CI_UART_SKIP_NO_DEVICE    — если 1 и порта нет: выход 0 (пропуск)
  CI_UART_STRICT_NO_DEVICE  — если 1 и порта нет: выход 2
  CI_UART_MARKERS_ORDERED   — если 1: требовать порядок как в файле (legacy)

Doc index: README.md
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path


def _print_fail_banner(msg: str) -> None:
    """Дублируем ошибку обычным текстом: в UI Gitea строка ::error:: часто обрезается."""
    line = "=" * 60
    print(f"\n{line}", flush=True)
    print("UART BOOT MARKERS — FAILED", flush=True)
    print(msg, flush=True)
    print(f"{line}\n", flush=True)


def _rx_stats_line(buf: str) -> str:
    nbyte = len(buf.encode("utf-8", errors="replace"))
    nlines = buf.count("\n") + (1 if buf and not buf.endswith("\n") else 0)
    hint = ""
    if nbyte == 0:
        hint = (
            " (ничего не прочитано: неверный порт/скорость, нет прав dialout, "
            "или бут-лог уже прошёл до открытия COM — нужен reset перед чтением; "
            "USE_RNG_DUMP=1 даёт бинарный поток без строк [WALLET])"
        )
    return f"RX: {nbyte} byte(s), ~{nlines} line(s){hint}"


def load_markers(path: Path) -> list[str]:
    lines: list[str] = []
    raw = path.read_text(encoding="utf-8", errors="replace").splitlines()
    for line in raw:
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        lines.append(s)
    if not lines:
        raise SystemExit(f"No markers in {path}")
    return lines


def main() -> int:
    ap = argparse.ArgumentParser(description="Wait for UART boot log markers (default: any order)")
    ap.add_argument(
        "--markers-file",
        default=os.environ.get("CI_UART_MARKERS_FILE", "scripts/ci/uart_boot_markers.txt"),
        help="Text file: one substring per line",
    )
    ap.add_argument("--port", default=os.environ.get("CI_UART_PORT", "/dev/ttyACM0"))
    ap.add_argument("--baud", type=int, default=int(os.environ.get("CI_UART_BAUD", "115200")))
    ap.add_argument(
        "--timeout",
        type=float,
        default=float(os.environ.get("CI_UART_BOOT_TIMEOUT_SEC", "120")),
        help="Total seconds to receive all markers",
    )
    ap.add_argument(
        "--ordered",
        action="store_true",
        help="Require markers in file order (legacy)",
    )
    ap.add_argument("--log-out", default=os.environ.get("CI_UART_LOG_OUT", "uart_output.log"))
    ap.add_argument(
        "--status-out", default=os.environ.get("CI_UART_STATUS_OUT", "uart_boot_status.txt")
    )
    args = ap.parse_args()

    ordered = args.ordered or os.environ.get("CI_UART_MARKERS_ORDERED", "").lower() in (
        "1",
        "true",
        "yes",
    )

    markers_path = Path(args.markers_file)
    if not markers_path.is_file():
        m = f"Markers file not found: {markers_path.resolve()}"
        _print_fail_banner(m)
        print(f"::error::{m}", flush=True)
        return 1

    markers = load_markers(markers_path)
    skip_no = os.environ.get("CI_UART_SKIP_NO_DEVICE", "1").lower() in ("1", "true", "yes")
    strict_no = os.environ.get("CI_UART_STRICT_NO_DEVICE", "0").lower() in ("1", "true", "yes")

    if not os.path.exists(args.port):
        msg = f"No serial device {args.port}"
        print(f"⚠️  {msg}", flush=True)
        Path(args.log_out).write_text(msg + "\n", encoding="utf-8")
        Path(args.status_out).write_text("SKIP: no device\n", encoding="utf-8")
        if strict_no:
            return 2
        if skip_no:
            return 0
        return 2

    try:
        import serial
    except ImportError:
        print("Install pyserial: pip install pyserial", file=sys.stderr)
        return 1

    ser = serial.Serial(port=args.port, baudrate=args.baud, timeout=0.25)
    time.sleep(0.2)
    try:
        ser.reset_input_buffer()
    except OSError:
        pass

    buffer = ""
    t0 = time.time()
    poll = 0.02

    mode = "strict order" if ordered else "any order (all must appear)"
    print(
        f"📡 Waiting for {len(markers)} markers, {mode} (timeout {args.timeout}s, port {args.port})...",
        flush=True,
    )

    if ordered:
        idx = 0
        try:
            while idx < len(markers):
                elapsed = time.time() - t0
                if elapsed > args.timeout:
                    pending = markers[idx]
                    tail = buffer[-2000:] if len(buffer) > 2000 else buffer
                    err = (
                        f"Timeout after {elapsed:.1f}s waiting for marker {idx + 1}/{len(markers)}: {pending!r}\n"
                        f"{_rx_stats_line(buffer)}\n"
                        f"--- log tail ---\n{tail}"
                    )
                    _print_fail_banner(err)
                    print(f"::error::{err}", flush=True)
                    Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
                    Path(args.status_out).write_text(f"FAIL: {err}\n", encoding="utf-8")
                    return 1

                try:
                    n = ser.in_waiting
                    if n:
                        chunk = ser.read(n)
                        text = chunk.decode("utf-8", errors="replace")
                        buffer += text
                        sys.stdout.write(text)
                        sys.stdout.flush()
                except OSError as e:
                    _print_fail_banner(f"UART read error: {e}")
                    print(f"::error::UART read error: {e}", flush=True)
                    Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
                    Path(args.status_out).write_text(f"FAIL: {e}\n", encoding="utf-8")
                    return 1

                while idx < len(markers) and markers[idx] in buffer:
                    print(f"\n✓ Marker {idx + 1}/{len(markers)}: {markers[idx]!r}", flush=True)
                    idx += 1

                time.sleep(poll)

            Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
            Path(args.status_out).write_text("OK\n", encoding="utf-8")
            print(f"\n✅ All {len(markers)} boot markers seen (ordered). Log → {args.log_out}", flush=True)
            return 0
        finally:
            ser.close()

    # Unordered: all markers must appear somewhere in buffer
    pending = set(markers)
    try:
        while pending:
            elapsed = time.time() - t0
            if elapsed > args.timeout:
                tail = buffer[-2000:] if len(buffer) > 2000 else buffer
                missing = sorted(pending)
                err = (
                    f"Timeout after {elapsed:.1f}s — missing {len(missing)} marker(s): {missing!r}\n"
                    f"{_rx_stats_line(buffer)}\n"
                    f"--- log tail ---\n{tail}"
                )
                _print_fail_banner(err)
                print(f"::error::{err}", flush=True)
                Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
                Path(args.status_out).write_text(f"FAIL: {err}\n", encoding="utf-8")
                return 1

            try:
                n = ser.in_waiting
                if n:
                    chunk = ser.read(n)
                    text = chunk.decode("utf-8", errors="replace")
                    buffer += text
                    sys.stdout.write(text)
                    sys.stdout.flush()
            except OSError as e:
                _print_fail_banner(f"UART read error: {e}")
                print(f"::error::UART read error: {e}", flush=True)
                Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
                Path(args.status_out).write_text(f"FAIL: {e}\n", encoding="utf-8")
                return 1

            found_now = [m for m in pending if m in buffer]
            for m in found_now:
                pending.discard(m)
                print(f"\n✓ Seen: {m!r} ({len(pending)} left)", flush=True)

            time.sleep(poll)

        Path(args.log_out).write_text(buffer, encoding="utf-8", errors="replace")
        Path(args.status_out).write_text("OK\n", encoding="utf-8")
        print(f"\n✅ All {len(markers)} service markers present (any order). Log → {args.log_out}", flush=True)
        return 0
    finally:
        ser.close()


if __name__ == "__main__":
    raise SystemExit(main())
