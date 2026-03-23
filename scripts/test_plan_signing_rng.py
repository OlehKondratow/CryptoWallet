#!/usr/bin/env python3
"""
Test plan: Bitcoin transaction signing + RNG (including DIEHARDER file workflow).

Documentation index: repository README.md (maps this script to firmware + docs).

Print markdown to stdout or write to a file (run from project root):

  python3 scripts/test_plan_signing_rng.py
  python3 scripts/test_plan_signing_rng.py --write documentation/generated/testing-plan-signing-rng.md

``make docs`` invokes this script before MkDocs build.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
from textwrap import dedent


PLAN_SECTIONS = [
    {
        "title": "1. Scope",
        "items": [
            "Bitcoin-style signing path m/44'/0'/0'/0/0 (project default).",
            "Message signed: SHA-256(recipient|amount|currency) then ECDSA secp256k1 compact 64-byte sig — not a raw Bitcoin tx.",
            "RNG: STM32 TRNG + software pool (see documentation/03-cryptography-and-signing.md).",
        ],
    },
    {
        "title": "2. Bitcoin signing — prerequisites",
        "items": [
            "Build: USE_CRYPTO_SIGN=1, USE_TEST_SEED=1 for known vector (dev only).",
            "Flash firmware; Ethernet or WebUSB per documentation/04-http-and-webusb.md.",
            "UART 115200 for logs (optional but recommended).",
        ],
    },
    {
        "title": "3. Bitcoin signing — functional tests",
        "items": [
            "**T-SIG-01** POST /tx valid JSON → UART: TX enqueued, TX recv; display shows pending/sign flow.",
            "**T-SIG-02** Short USER (PC13) → Confirm → UART: Signed OK (with test seed).",
            "**T-SIG-03** GET /tx/signed → JSON status signed + hex sig 128 chars (64 bytes).",
            "**T-SIG-04** Long USER → Reject → no valid signature / Reject in log.",
            "**T-SIG-05** No press 30 s → Timeout.",
            "**T-SIG-06** Invalid JSON / bad recipient → TX invalid.",
            "**T-SIG-07** WebUSB: scripts/test_usb_sign.py ping then sign (USE_WEBUSB=1).",
            "**T-SIG-08** Cross-check: same inputs + same seed → reproducible signature (deterministic k only if RFC6979 path; verify against project crypto).",
        ],
    },
    {
        "title": "4. Bitcoin signing — security / negative",
        "items": [
            "**T-SEC-01** Without USE_TEST_SEED and without get_wallet_seed impl → No seed after confirm.",
            "**T-SEC-02** No private key or seed in UART/HTTP responses (inspect logs).",
        ],
    },
    {
        "title": "5. RNG — design checks",
        "items": [
            "**T-RNG-01** HAL_RNG / TRNG enabled in build; no fallback-only path in production config.",
            "**T-RNG-02** After reset, random_buffer output differs across power cycles (spot check).",
            "**T-RNG-03** Code review: memzero on seed/privkey paths after use (task_sign / crypto_wallet).",
        ],
    },
    {
        "title": "6. RNG — DIEHARDER statistical tests",
        "items": [
            "Purpose: detect gross bias / correlation; not a crypto certification.",
            "**Prerequisite firmware**: binary-only UART stream (no text). Add USE_RNG_DUMP or similar if not present.",
            "**Capture file**: scripts/capture_rng_uart.py --port ... --out rng.bin --bytes 134217728 (≥128 MiB recommended).",
            "**Run**: scripts/run_dieharder.py --file rng.bin (requires `dieharder` package).",
            "**Pass criteria**: interpret p-values; investigate clusters of failures; re-run with larger file if borderline.",
        ],
    },
    {
        "title": "7. Traceability",
        "items": [
            "Design reference: documentation/03-cryptography-and-signing.md, 04-http-and-webusb.md, 06-integrity-rng-verification.md.",
        ],
    },
]


def render_markdown() -> str:
    lines = [
        "# Test plan: Bitcoin signing & RNG",
        "",
        f"_Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}_",
        "",
        dedent(
            """
            This plan complements project documentation. Automated hooks are minimal;
            mark items manually in your tracker or checklist.
            """
        ).strip(),
        "",
    ]
    for sec in PLAN_SECTIONS:
        lines.append(f"## {sec['title']}")
        lines.append("")
        for it in sec["items"]:
            lines.append(f"- {it}")
        lines.append("")
    lines.append("## DIEHARDER file checklist")
    lines.append("")
    for it in [
        "[ ] Firmware outputs **only** raw bytes on the chosen interface.",
        "[ ] UART baud matches capture script (default 115200).",
        "[ ] `scripts/capture_rng_uart.py` wrote expected byte count.",
        "[ ] `dieharder` installed: `dieharder -l` works.",
        "[ ] `scripts/run_dieharder.py --file rng.bin` completed; results archived.",
    ]:
        lines.append(it)
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    ap = argparse.ArgumentParser(description="Emit signing + RNG test plan (Markdown)")
    ap.add_argument(
        "--write",
        "-w",
        metavar="PATH",
        help="Write markdown to file instead of stdout",
    )
    args = ap.parse_args()
    text = render_markdown()
    if args.write:
        with open(args.write, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"Wrote {args.write}", file=__import__("sys").stderr)
    else:
        print(text)


if __name__ == "__main__":
    main()
