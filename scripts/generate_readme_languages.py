#!/usr/bin/env python3
"""
Generate canonical English README from base README source.
Legacy RU/PL generation has been removed in favor of documentation/lang/* translations.

Non-English replacement strings live in UTF-8 JSON (see scripts/i18n/readme_translations.json)
so this file stays ASCII-only.

Usage:
    python3 scripts/generate_readme_languages.py

Output:
    - README.md (English - canonical)
"""

import json
import os
import re
from pathlib import Path

_I18N_JSON = Path(__file__).resolve().parent / "i18n" / "readme_translations.json"
_TRANSLATIONS_CACHE = None

def _load_translation_data():
    """Load EN->RU and EN->PL phrase pairs and README link-line suffixes from JSON."""
    global _TRANSLATIONS_CACHE
    if _TRANSLATIONS_CACHE is None:
        with open(_I18N_JSON, encoding="utf-8") as f:
            raw = json.load(f)
        _TRANSLATIONS_CACHE = {
            "ru": [tuple(p) for p in raw["ru"]],
            "pl": [tuple(p) for p in raw["pl"]],
            "link_line_suffix": raw.get("link_line_suffix", {}),
        }
    return _TRANSLATIONS_CACHE


def translate_readme(readme_path, lang):
    """Translate README to specified language."""
    with open(readme_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # RU/PL README variants were removed; keep canonical English content only.
    if lang != 'en':
        return content

    # Protect code blocks and links before translation
    code_blocks = []
    links = []

    # Extract code blocks
    def extract_code(match):
        code_blocks.append(match.group(0))
        return f"<<<CODE_BLOCK_{len(code_blocks)-1}>>>"

    content = re.sub(r'```[^`]*```', extract_code, content, flags=re.DOTALL)

    # Extract markdown links to preserve them
    def extract_link(match):
        links.append(match.group(0))
        return f"<<<LINK_{len(links)-1}>>>"

    content = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', extract_link, content)

    # Apply translations (longer phrases first: list order in JSON)
    data = _load_translation_data()
    translations = data.get(lang, [])
    for original, translated in translations:
        content = content.replace(original, translated)

    # Restore links
    for i, link in enumerate(links):
        content = content.replace(f"<<<LINK_{i}>>>", link)

    # Restore code blocks
    for i, code in enumerate(code_blocks):
        content = content.replace(f"<<<CODE_BLOCK_{i}>>>", code)

    return content


def main():
    readme_path = Path('README.md')

    if not readme_path.exists():
        print(f"Error: {readme_path} not found")
        return

    print("Generating canonical English README...")
    translated = translate_readme(readme_path, 'en')
    output_file = 'README.md'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(translated)
    print(f"OK Generated {output_file} (English)")
    print("\nDone.")


if __name__ == '__main__':
    main()
