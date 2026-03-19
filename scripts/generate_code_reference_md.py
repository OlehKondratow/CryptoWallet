#!/usr/bin/env python3
"""
Generate docs_src/reference-code.md from file-level Doxygen headers and Python docstrings.

See docs_src/code-doc-generation.md for design and alternatives.
"""

from __future__ import annotations

import argparse
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

# Only these @tags start a new block (avoids treating @c ... as a tag at line start).
ALLOWED_DOXY_TAGS = frozenset(
    {"file", "brief", "details", "note", "warning", "attention", "copyright"}
)

BANNER_RE = re.compile(r"^[\s*\-]+$")


def strip_c_comment_line(raw: str) -> str | None:
    """Turn '  * @brief foo' into '@brief foo'; skip banners / empty."""
    s = raw.rstrip("\n")
    s = s.strip()
    if s.startswith("/**") or s.startswith("*/"):
        return None
    if s.startswith("*"):
        s = s[1:].strip()
    if not s or BANNER_RE.match(s):
        return None
    return s


def extract_first_doxygen_block(text: str) -> list[str] | None:
    start = text.find("/**")
    if start == -1:
        return None
    end = text.find("*/", start + 3)
    if end == -1:
        return None
    inner = text[start + 3 : end]
    out: list[str] = []
    for line in inner.splitlines():
        cleaned = strip_c_comment_line(line)
        if cleaned is not None:
            out.append(cleaned)
    return out or None


TAG_LINE_RE = re.compile(r"^@([a-zA-Z_][a-zA-Z0-9_]*)\s*(.*)$")


def parse_doxygen_lines(lines: list[str]) -> dict[str, str]:
    """Split into @file, @brief, @details, ... (multiline values)."""
    fields: dict[str, list[str]] = {}
    current: str | None = None
    buf: list[str] = []

    def flush() -> None:
        nonlocal current, buf
        if current is not None:
            val = "\n".join(buf).strip()
            fields.setdefault(current, []).append(val)
        current = None
        buf = []

    for line in lines:
        m = TAG_LINE_RE.match(line)
        if m:
            tag = m.group(1)
            rest = m.group(2)
            if tag in ALLOWED_DOXY_TAGS:
                flush()
                current = tag
                buf = [rest] if rest.strip() else []
                continue
        if current is not None:
            buf.append(line)

    flush()

    # Join repeated tags (shouldn't happen often)
    return {k: "\n\n".join(v).strip() for k, v in fields.items()}


def extract_python_module_docstring(text: str) -> str | None:
    """First top-level triple-quoted string after shebang / encoding / comments."""
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        if line.startswith("#!") or line.startswith("# -*-") or line.startswith("# coding"):
            i += 1
            continue
        if line == "" or line.startswith("#"):
            i += 1
            continue
        break
    if i >= len(lines):
        return None
    line = lines[i]
    for quote in ('"""', "'''"):
        if line.startswith(quote):
            if line.endswith(quote) and len(line) >= 3 + len(quote) and line.rstrip().endswith(quote):
                inner = line[len(quote) : line.rfind(quote)]
                return inner.strip() or None
            block = [line[len(quote) :]]
            i += 1
            while i < len(lines):
                if quote in lines[i]:
                    block.append(lines[i].split(quote, 1)[0])
                    return "\n".join(block).strip() or None
                block.append(lines[i])
                i += 1
            return None
    return None


def collect_sources(root: Path, extra_globs: list[str]) -> list[Path]:
    paths: set[Path] = set()
    default_globs = [
        "Core/Src/*.c",
        "Core/Inc/*.h",
        "Src/*.c",
        "Drivers/**/*.h",
        "scripts/*.py",
    ]
    for pattern in default_globs + extra_globs:
        for p in root.glob(pattern):
            if p.is_file():
                paths.add(p.resolve())
    return sorted(paths, key=lambda p: str(p.relative_to(root)).lower())


def file_section(root: Path, path: Path) -> str | None:
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8", errors="replace")
    lines: list[str] = []

    if path.suffix.lower() == ".py":
        doc = extract_python_module_docstring(text)
        if not doc:
            return None
        lines.append(f"### `{rel}`\n")
        lines.append("```text\n")
        lines.append(doc)
        lines.append("\n```\n")
        return "".join(lines)

    doxy_lines = extract_first_doxygen_block(text)
    if not doxy_lines:
        return None
    fields = parse_doxygen_lines(doxy_lines)
    if not fields:
        return None

    title = fields.get("file", rel.name)
    lines.append(f"### `{rel}`\n")
    if "brief" in fields:
        lines.append(f"**@brief:** {fields['brief']}\n\n")
    for key in ("details", "note", "warning", "attention"):
        if key in fields:
            lines.append(f"**@{key}**\n\n")
            lines.append(fields[key] + "\n\n")
    if "copyright" in fields:
        lines.append(f"*@copyright* {fields['copyright']}\n\n")
    return "".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate reference-code.md from sources.")
    ap.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("docs_src/reference-code.md"),
        help="Output Markdown path (default: docs_src/reference-code.md)",
    )
    ap.add_argument(
        "--root",
        type=Path,
        default=Path("."),
        help="Repository root (default: .)",
    )
    ap.add_argument(
        "--glob",
        action="append",
        default=[],
        metavar="PATTERN",
        help="Extra glob relative to root (repeatable)",
    )
    ap.add_argument(
        "--stdout",
        action="store_true",
        help="Print to stdout instead of writing file",
    )
    args = ap.parse_args()
    root = args.root.resolve()
    paths = collect_sources(root, args.glob)

    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    out: list[str] = []
    out.append("# Code reference (auto-generated)\n\n")
    out.append("!!! warning \"Generated file\"\n")
    out.append("    Do not edit by hand. Regenerate:\n\n")
    out.append("    ```bash\n")
    out.append("    make docs-code-md\n")
    out.append("    # or: python3 scripts/generate_code_reference_md.py\n")
    out.append("    ```\n\n")
    out.append(f"**Generated:** {now}  \n")
    out.append(
        "**Source:** file headers (`@file` / `@brief` / `@details` in `/** ... */`) and "
        "Python module docstrings in `scripts/*.py`.  \n"
    )
    out.append("**Design:** [code-doc-generation](code-doc-generation.md)\n\n")

    sections: dict[str, list[str]] = {
        "Firmware — Core/Src": [],
        "Firmware — Core/Inc": [],
        "Firmware — Src": [],
        "Drivers": [],
        "Scripts": [],
        "Other": [],
    }

    for path in paths:
        rel = path.relative_to(root)
        part = file_section(root, path)
        if not part:
            continue
        s = str(rel).replace("\\", "/")
        if s.startswith("Core/Src/"):
            key = "Firmware — Core/Src"
        elif s.startswith("Core/Inc/"):
            key = "Firmware — Core/Inc"
        elif s.startswith("Src/"):
            key = "Firmware — Src"
        elif s.startswith("scripts/"):
            key = "Scripts"
        elif s.startswith("Drivers/"):
            key = "Drivers"
        else:
            key = "Other"
        sections[key].append(part)

    for title, chunks in sections.items():
        if not chunks:
            continue
        out.append(f"## {title}\n\n")
        out.extend(chunks)

    if len(out) <= 8:
        out.append("_No extractable headers found._\n")

    body = "".join(out)
    if args.stdout:
        sys.stdout.write(body)
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(body, encoding="utf-8")
    print(f"Wrote {args.output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
