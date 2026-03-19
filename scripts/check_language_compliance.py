#!/usr/bin/env python3
"""
Check language compliance for all docs_src/ files.
Validates:
- File naming convention (*.md, *_ru.md, *_pl.md)
- Language content detection (English, Russian, Polish)
- Required <brief> tags
- Encoding (UTF-8)
"""

import os
import re
from pathlib import Path
from collections import defaultdict

# Cyrillic and Polish character ranges
CYRILLIC_CHARS = re.compile(r'[а-яёА-ЯЁ]')
POLISH_CHARS = re.compile(r'[ąćęłńóśźżĄĆĘŁŃÓŚŹŻ]')

def detect_language(text):
    """Detect language based on character content."""
    cyrillic_count = len(CYRILLIC_CHARS.findall(text))
    polish_count = len(POLISH_CHARS.findall(text))
    
    # Check for doxygen commands (language-neutral)
    if text.count(r'\page') > 0 or text.count(r'\related') > 0:
        # These are present in all versions
        pass
    
    if cyrillic_count > 10:
        return "russian"
    elif polish_count > 10:
        return "polish"
    else:
        return "english"

def check_file(filepath):
    """Check a single file for compliance."""
    issues = []
    filename = filepath.name
    
    # Check UTF-8 encoding
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        return ["❌ NOT UTF-8 ENCODED"]
    
    # Determine expected language from filename
    if filename.endswith('_ru.md'):
        expected_lang = 'russian'
        base = filename[:-6]
    elif filename.endswith('_pl.md'):
        expected_lang = 'polish'
        base = filename[:-6]
    else:
        expected_lang = 'english'
        base = filename[:-3]
    
    # Detect actual language
    detected_lang = detect_language(content)
    
    # Validate language match
    if expected_lang != detected_lang:
        issues.append(f"⚠️ Language mismatch: expected {expected_lang}, detected {detected_lang}")
    
    # Check for required Doxygen
    if r'\page' not in content:
        issues.append("❌ Missing \\page directive")
    
    # Check for brief tag
    if '<brief>' not in content or '</brief>' not in content:
        issues.append("❌ Missing <brief>...</brief> tags")
    
    # Check brief tag content is not empty
    brief_match = re.search(r'<brief>(.*?)</brief>', content, re.DOTALL)
    if brief_match:
        brief_content = brief_match.group(1).strip()
        if len(brief_content) < 20:
            issues.append(f"⚠️ Brief tag too short ({len(brief_content)} chars)")
        if '\n' in brief_content:
            issues.append("⚠️ Brief tag contains newlines (should be single paragraph)")
    
    return issues if issues else ["✅ OK"]

def main():
    docs_dir = Path("docs_src")
    
    if not docs_dir.exists():
        print("Error: docs_src/ directory not found")
        return
    
    files = sorted([f for f in docs_dir.glob("*.md")])
    
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  LANGUAGE COMPLIANCE CHECK                                   ║")
    print("╚════════════════════════════════════════════════════════════════╝\n")
    
    # Group by base name
    base_groups = defaultdict(dict)
    for f in files:
        if f.name.endswith('_ru.md'):
            base = f.name[:-6]
            lang = 'ru'
        elif f.name.endswith('_pl.md'):
            base = f.name[:-6]
            lang = 'pl'
        else:
            base = f.name[:-3]
            lang = 'en'
        base_groups[base][lang] = f
    
    # Check each file
    problem_files = []
    
    for base in sorted(base_groups.keys()):
        print(f"\n{base}")
        print("─" * 65)
        
        for lang in ['en', 'ru', 'pl']:
            lang_labels = {'en': '🇬🇧 EN', 'ru': '🇷🇺 RU', 'pl': '🇵🇱 PL'}
            lang_label = lang_labels[lang]
            
            if lang in base_groups[base]:
                filepath = base_groups[base][lang]
                issues = check_file(filepath)
                
                if issues == ['✅ OK']:
                    print(f"  {lang_label}: {issues[0]}")
                else:
                    print(f"  {lang_label}:")
                    for issue in issues:
                        print(f"    {issue}")
                    problem_files.append((base, lang, issues))
            else:
                print(f"  {lang_label}: ❌ FILE NOT FOUND")
                problem_files.append((base, lang, ["❌ FILE NOT FOUND"]))
    
    # Summary
    print(f"\n{'='*65}")
    print(f"TOTAL FILES: {len(files)}")
    print(f"ISSUES FOUND: {len(problem_files)}")
    
    if problem_files:
        print(f"\n{'='*65}")
        print("NEEDS ATTENTION:\n")
        for base, lang, issues in problem_files:
            lang_label = {'en': 'EN', 'ru': 'RU', 'pl': 'PL'}[lang]
            print(f"  {base} ({lang_label})")
            for issue in issues:
                print(f"    {issue}")

if __name__ == '__main__':
    main()
