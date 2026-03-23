#!/usr/bin/env python3
"""
Update README.md "Project Structure" from Doxygen XML and/or emit Markdown per file.

Doxygen does not output Markdown natively (no GENERATE_MARKDOWN). This script:
1. Parses docs_doxygen/xml/ (run: doxygen Doxyfile).
2. Extracts @brief (short) and @details (detailed) for each file/compound.
3. Updates root README.md: replaces the table in "## Project Structure" with generated rows.
4. Optionally writes docs_doxygen/md/<file>.md with brief + details (Markdown output).

Usage:
  doxygen Doxyfile
  python3 scripts/update_readme.py
  python3 scripts/update_readme.py --readme README.md --xml docs_doxygen/xml
  python3 scripts/update_readme.py --md-dir docs_doxygen/md   # emit .md per source file
"""

from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def strip_para(text: str) -> str:
    """One-line summary from XML para/mixed content."""
    if not text:
        return ""
    text = re.sub(r"\s+", " ", text).strip()
    return text[:200] + ("..." if len(text) > 200 else "")


def inner_text(node: ET.Element | None) -> str:
    """Recursively get text and tail, strip XML tags."""
    if node is None:
        return ""
    parts: list[str] = []
    if node.text:
        parts.append(node.text)
    for child in node:
        parts.append(inner_text(child))
        if child.tail:
            parts.append(child.tail)
    return "".join(parts).strip()


def parse_brief(compound: ET.Element) -> str:
    """Extract brief description for a compound (file/struct/etc.)."""
    brief = compound.find("briefdescription")
    if brief is None:
        return ""
    return strip_para(inner_text(brief))


def parse_details(compound: ET.Element) -> str:
    """Extract detailed description (may contain paras, lists)."""
    det = compound.find("detaileddescription")
    if det is None:
        return ""
    return inner_text(det).strip()


def iter_compounds(xml_dir: Path):
    """Yield (refid, kind, name, path, brief, details) from index.xml + compound files."""
    index_path = xml_dir / "index.xml"
    if not index_path.is_file():
        return
    tree = ET.parse(index_path)
    root = tree.getroot()
    for comp in root.findall(".//compound"):
        kind = comp.get("kind")
        refid = comp.get("refid")
        name = comp.get("name", "")
        if kind not in ("file", "dir"):
            continue
        if not refid:
            continue
        comp_path = xml_dir / f"{refid}.xml"
        if not comp_path.is_file():
            yield (refid, kind, name, name, "", "")
            continue
        try:
            sub = ET.parse(comp_path).getroot()
            c = sub.find("compounddef")
            if c is None:
                continue
            loc = c.find("location")
            path = loc.get("file", name) if loc is not None else name
            brief = parse_brief(c)
            details = parse_details(c)
            yield (refid, kind, name, path, brief, details)
        except ET.ParseError:
            yield (refid, kind, name, name, "", "")


def build_table(entries: list[tuple[str, str]]) -> str:
    """Markdown table: | Module | Brief |"""
    lines = ["| Module | Brief |", "|--------|-------|"]
    for path, brief in entries:
        path_esc = path.replace("|", "\\|")
        brief_esc = (brief or "(no description)").replace("|", "\\|").replace("\n", " ")
        lines.append(f"| `{path_esc}` | {brief_esc} |")
    return "\n".join(lines)


def update_readme_section(
    readme_path: Path,
    section_title: str,
    new_content: str,
    marker_comment: str = "<!-- DOXYGEN_PROJECT_STRUCTURE -->",
) -> bool:
    """Replace or insert '## section_title' block with new_content. Returns True if changed."""
    if not readme_path.is_file():
        return False
    text = readme_path.read_text(encoding="utf-8", errors="replace")
    marker = f"## {section_title}"
    lines = text.split("\n")
    start_i = None
    for i, line in enumerate(lines):
        if marker in line or marker_comment in line:
            start_i = i
            break
    if start_i is None:
        append = f"\n\n{marker}\n\n{new_content}\n"
        readme_path.write_text(text.rstrip() + append, encoding="utf-8")
        return True
    end_i = start_i + 1
    while end_i < len(lines):
        if re.match(r"^##\s+", lines[end_i].strip()) and end_i > start_i + 1:
            break
        end_i += 1
    new_lines = lines[: start_i + 1] + [""] + [new_content] + [""] + lines[end_i:]
    new_text = "\n".join(new_lines)
    if new_text != text:
        readme_path.write_text(new_text, encoding="utf-8")
        return True
    return False


def emit_markdown_files(
    xml_dir: Path,
    md_dir: Path,
    entries: list[tuple[str, str, str]],
) -> None:
    """Write one .md per file: title, brief, details."""
    md_dir.mkdir(parents=True, exist_ok=True)
    for path, brief, details in entries:
        safe = path.replace("/", "_").replace("\\", "_").replace(".", "_")
        out = md_dir / f"{safe}.md"
        body = [f"# {path}", ""]
        if brief:
            body.append("## Brief")
            body.append("")
            body.append(brief)
            body.append("")
        if details:
            body.append("## Detailed")
            body.append("")
            body.append(details)
            body.append("")
        out.write_text("\n".join(body), encoding="utf-8")
    print(f"Wrote {len(entries)} Markdown files to {md_dir}", file=sys.stderr)


def main() -> int:
    ap = argparse.ArgumentParser(description="Update README from Doxygen XML; optional MD output.")
    ap.add_argument("--xml", type=Path, default=Path("docs_doxygen/xml"), help="Doxygen XML output dir")
    ap.add_argument("--readme", type=Path, default=Path("README.md"), help="Root README.md")
    ap.add_argument("--section", default="Project Structure", help="Section title to replace")
    ap.add_argument("--md-dir", type=Path, default=None, help="Emit one .md per file into this dir")
    ap.add_argument("--dry-run", action="store_true", help="Print table, do not write README")
    args = ap.parse_args()
    root = Path(".").resolve()
    xml_dir = (root / args.xml).resolve()
    if not xml_dir.is_dir():
        print(f"XML dir not found: {xml_dir}. Run: doxygen Doxyfile", file=sys.stderr)
        return 1
    entries_full: list[tuple[str, str, str]] = []
    table_entries: list[tuple[str, str]] = []
    for _refid, kind, name, path, brief, details in iter_compounds(xml_dir):
        if kind != "file":
            continue
        # Only code headers/sources should go into the "Project Structure" table.
        # We intentionally ignore Markdown pages (manual lives in `documentation/`, built with MkDocs).
        if not str(path).lower().endswith((".c", ".h")):
            continue
        # Prefer relative path for display
        disp = path if not path.startswith(str(root)) else path
        entries_full.append((disp, brief, details))
        table_entries.append((disp, brief))
    if not table_entries:
        print("No file compounds found in XML.", file=sys.stderr)
        return 0
    table_entries.sort(key=lambda x: x[0].lower())
    table_md = build_table(table_entries)
    if args.dry_run:
        print(table_md)
        return 0
    readme_path = root / args.readme
    if readme_path.is_file():
        updated = update_readme_section(readme_path, args.section, table_md)
        if updated:
            print(f"Updated {readme_path} section '{args.section}'", file=sys.stderr)
        else:
            print(f"Section '{args.section}' not found or unchanged", file=sys.stderr)
    else:
        # Create minimal README with section
        content = "# CryptoWallet\n\n## Project Structure\n\n" + table_md + "\n"
        readme_path.write_text(content, encoding="utf-8")
        print(f"Created {readme_path} with Project Structure table", file=sys.stderr)
    if args.md_dir:
        md_dir = root / args.md_dir
        entries_full.sort(key=lambda x: x[0].lower())
        emit_markdown_files(xml_dir, md_dir, entries_full)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
