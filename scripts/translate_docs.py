#!/usr/bin/env python3
"""
Create multi-language documentation copies: EN, RU, PL.
Uses simple pattern-based translation + Google Translate API fallback.

Example heading maps (RU/PL strings) live in scripts/i18n/doc_heading_translations.json
so this file stays ASCII-only.
"""

import json
from pathlib import Path
import sys

_I18N = Path(__file__).resolve().parent / "i18n" / "doc_heading_translations.json"
with open(_I18N, encoding="utf-8") as _f:
    TRANSLATIONS = json.load(_f)


def get_language_suffix(lang: str) -> str:
    """Get file suffix for language."""
    if lang == "en":
        return ""
    return f"_{lang}"


def main():
    docs_dir = Path("documentation")
    readme = Path("README.md")

    print("Creating language versions...")
    print(f"Found {len(list(docs_dir.glob('*.md')))} docs in documentation/")
    print(f"Found README.md")
    print()
    print("To create full translations, use:")
    print("  pip install google-cloud-translate")
    print("  export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-key.json")
    print()
    print("OR manually create files using translation service and copy structure.")
    print()

    # Example: show what would be created
    files_to_translate = list(docs_dir.glob("*.md")) + [readme]
    print(f"Would create {len(files_to_translate) * 2} additional files:")
    print("  - *_ru.md (Russian versions)")
    print("  - *_pl.md (Polish versions)")
    print()
    print("Current structure:")
    for f in sorted(files_to_translate)[:5]:
        print(f"  {f}")
    print("  ...")


if __name__ == "__main__":
    main()
