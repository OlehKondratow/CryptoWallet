#!/usr/bin/env python3
"""
Generate README index from hand-written educational docs in `docs_src/`.

Rules:
1) Scan `docs_src/*.md`
2) Extract short text from `## Краткий обзор` (or `## Summary`)
3) Create a Markdown table with links to each full doc
4) Inject the table into root `README.md` between markers
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


SUMMARY_HEADING_RE = re.compile(r"^##\s+(Краткий обзор|Summary)\s*$", re.MULTILINE)
NEXT_LEVEL_2_HEADING_RE = re.compile(r"^##\s+", re.MULTILINE)


def normalize_ws(s: str) -> str:
    s = re.sub(r"[ \t]+", " ", s)
    s = re.sub(r"\n{3,}", "\n\n", s)
    return s.strip()


def extract_first_paragraph(s: str) -> str:
    s = normalize_ws(s)
    if not s:
        return ""
    # Split by blank line; if nothing, fall back to first line.
    parts = re.split(r"\n\s*\n", s, maxsplit=1)
    if parts:
        return parts[0].strip()
    return s.splitlines()[0].strip()


def extract_summary(md_text: str) -> str:
    """
    Extract summary from `## Краткий обзор` / `## Summary`.
    If missing, use the first paragraph after the first top-level title.
    """
    m = SUMMARY_HEADING_RE.search(md_text)
    if m:
        start = m.end()
        m2 = NEXT_LEVEL_2_HEADING_RE.search(md_text, pos=start)
        end = m2.start() if m2 else len(md_text)
        block = md_text[start:end].strip()
        # Keep it short: 1-2 sentences / first paragraph.
        return extract_first_paragraph(block)

    # Fallback: find first '# ' heading and use first paragraph after it.
    first_title = re.search(r"^#\s+.*$", md_text, flags=re.MULTILINE)
    start = first_title.end() if first_title else 0
    remaining = md_text[start:]
    return extract_first_paragraph(remaining)


def iter_docs(docs_dir: Path) -> list[Path]:
    if not docs_dir.is_dir():
        raise FileNotFoundError(f"docs dir not found: {docs_dir}")
    return sorted(docs_dir.glob("*.md"), key=lambda p: p.name.lower())


def build_table(docs: list[Path], docs_dir: Path) -> str:
    rows: list[str] = []
    for p in docs:
        text = p.read_text(encoding="utf-8", errors="replace")
        summary = extract_summary(text)
        # Use filename stem as module id.
        module = p.stem
        # Escape pipe characters for Markdown table.
        summary_md = (summary or "(no summary)")
        # Markdown tables break on raw newlines; keep it single-line.
        summary_md = re.sub(r"\s+", " ", summary_md).strip()
        summary_md = summary_md.replace("|", "\\|")
        doc_link = f"{docs_dir.name}/{p.name}"
        rows.append(f"| [{module}]({doc_link}) | {summary_md} |")

    header = ["| Модуль | Краткий обзор |", "|--------|------------------|"]
    return "\n".join(header + rows)


def update_readme(readme_path: Path, section_title: str, table_md: str) -> bool:
    readme = readme_path.read_text(encoding="utf-8", errors="replace")

    start_marker = "<!-- DOXYGEN_DOCS_SRC_INDEX -->"
    end_marker = "<!-- /DOXYGEN_DOCS_SRC_INDEX -->"

    marker_block_re = re.compile(
        re.escape(start_marker) + r".*?" + re.escape(end_marker),
        flags=re.DOTALL,
    )

    section_block = (
        f"{section_title}\n"
        f"{start_marker}\n"
        f"{table_md}\n"
        f"{end_marker}"
    )

    if marker_block_re.search(readme):
        # Replace only the markers block content.
        new = marker_block_re.sub(start_marker + "\n" + table_md + "\n" + end_marker, readme)
        if new != readme:
            readme_path.write_text(new, encoding="utf-8")
            return True
        return False

    # If section isn't found, append it.
    if section_title not in readme:
        readme = readme.rstrip() + "\n\n" + section_block + "\n"
        readme_path.write_text(readme, encoding="utf-8")
        return True

    # Section exists but markers don't: append markers block after the title.
    # Best-effort: insert right after the first occurrence of the title line.
    idx = readme.find(section_title)
    if idx == -1:
        return False
    # Insert after that line.
    line_end = readme.find("\n", idx)
    if line_end == -1:
        line_end = len(readme)
    new = readme[: line_end + 1] + "\n" + section_block.split("\n", 1)[1] + "\n" + readme[line_end + 1 :]
    readme_path.write_text(new, encoding="utf-8")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description="Update README docs_src index.")
    ap.add_argument("--docs-dir", type=Path, default=Path("docs_src"))
    ap.add_argument("--readme", type=Path, default=Path("README.md"))
    ap.add_argument("--section-title", default="## Учебные разборы (docs_src)")
    ap.add_argument("--dry-run", action="store_true", help="Print table, do not write README")
    args = ap.parse_args()

    docs_dir: Path = args.docs_dir
    readme_path: Path = args.readme

    docs = iter_docs(docs_dir)
    table_md = build_table(docs, docs_dir)

    if args.dry_run:
        sys.stdout.write(table_md + "\n")
        return 0

    if not readme_path.is_file():
        raise FileNotFoundError(f"README not found: {readme_path}")

    changed = update_readme(readme_path, args.section_title, table_md)
    if changed:
        print(f"Updated {readme_path} ({args.section_title})", file=sys.stderr)
    else:
        print(f"No changes for {readme_path} ({args.section_title})", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

