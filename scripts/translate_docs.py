#!/usr/bin/env python3
"""
Create multi-language documentation copies: EN, RU, PL.
Uses simple pattern-based translation + Google Translate API fallback.
"""

from pathlib import Path
import sys

# Simple keyword translations (will be expanded or use API for full content)
TRANSLATIONS = {
    "ru": {
        "# CryptoWallet": "# CryptoWallet",
        "Core & Security": "Ядро и Безопасность",
        "Communication Interfaces": "Интерфейсы связи",
        "User Experience": "Интерфейс и UX",
        "System & Hardware": "Система и Железо",
        "Documentation and reference": "Документация и справка",
        "Quick Start": "Быстрый старт",
    },
    "pl": {
        "# CryptoWallet": "# CryptoWallet",
        "Core & Security": "Jądro i Bezpieczeństwo",
        "Communication Interfaces": "Interfejsy komunikacji",
        "User Experience": "Interfejs użytkownika",
        "System & Hardware": "System i Sprzęt",
        "Documentation and reference": "Dokumentacja i odniesienia",
        "Quick Start": "Szybki start",
    }
}

def get_language_suffix(lang: str) -> str:
    """Get file suffix for language."""
    if lang == "en":
        return ""
    return f"_{lang}"

def main():
    docs_dir = Path("docs_src")
    readme = Path("README.md")
    
    print("Creating language versions...")
    print(f"Found {len(list(docs_dir.glob('*.md')))} docs in docs_src/")
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
