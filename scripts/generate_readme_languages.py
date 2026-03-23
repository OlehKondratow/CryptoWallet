#!/usr/bin/env python3
"""
Generate README in multiple languages (EN, RU, PL) from base English README.
Translates section headers and descriptive text while preserving links and code.

Non-English replacement strings live in UTF-8 JSON (see scripts/i18n/readme_translations.json)
so this file stays ASCII-only.

Usage:
    python3 scripts/generate_readme_languages.py

Output:
    - README.md (English - unchanged)
    - README_ru.md (Russian)
    - README_pl.md (Polish)
"""

import json
import os
import re
from pathlib import Path

_I18N_JSON = Path(__file__).resolve().parent / "i18n" / "readme_translations.json"
_TRANSLATIONS_CACHE = None

# README language line for Russian (UTF-8 escapes keep this module free of Cyrillic literals)
_RU_README_LINK_LINE = "- \U0001f1f7\U0001f1fa [\u0420\u0443\u0441\u0441\u043a\u0438\u0439](README_ru.md)"


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

    if lang == 'en':
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

    # Update language links in header
    # First, remove any "(this file)" or translated variants from all links
    content = re.sub(r'(- 🇬🇧 \[English\]\(README\.md\))\s+\([^)]+\)', r'\1', content)
    content = re.sub(
        re.escape(_RU_README_LINK_LINE) + r'\s+\([^)]+\)',
        _RU_README_LINK_LINE,
        content,
    )
    content = re.sub(r'(- 🇵🇱 \[Polski\]\(README_pl\.md\))\s+\([^)]+\)', r'\1', content)

    # Then add the appropriate "(this file)" indicator for current language
    suffixes = data.get("link_line_suffix", {})
    if lang == 'ru':
        suf = suffixes.get('ru', ' (this file)')
        content = content.replace(_RU_README_LINK_LINE, _RU_README_LINK_LINE + suf)
    elif lang == 'pl':
        suf = suffixes.get('pl', ' (this file)')
        content = re.sub(
            r'- 🇵🇱 \[Polski\]\(README_pl\.md\)',
            '- 🇵🇱 [Polski](README_pl.md)' + suf,
            content
        )

    return content


def main():
    readme_path = Path('README.md')

    if not readme_path.exists():
        print(f"Error: {readme_path} not found")
        return

    print("Generating README in multiple languages...")

    # Generate for all languages
    for lang, lang_name in [('en', 'English'), ('ru', 'Russian'), ('pl', 'Polish')]:
        translated = translate_readme(readme_path, lang)

        if lang == 'en':
            output_file = 'README.md'
        else:
            output_file = f'README_{lang}.md'

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(translated)

        print(f"OK Generated {output_file} ({lang_name})")

    print("\nAll README files generated successfully.")


if __name__ == '__main__':
    main()
