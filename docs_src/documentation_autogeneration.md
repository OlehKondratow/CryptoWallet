# Documentation Auto-Generation System

## Overview

The CryptoWallet project includes a complete documentation auto-generation system built on Python scripts and Make targets. This system generates and maintains:

- **reference-code.md** — Auto-generated code reference from Doxygen comments
- **Language translations** — Polish and Russian README versions
- **HTML documentation** — MkDocs-based website
- **Index synchronization** — docs_src/ structure updates

## Python Generation Scripts

### `scripts/generate_code_reference_md.py`

Generates `docs_src/reference-code.md` from Doxygen comments in source files.

**Features:**
- Parses `@file`, `@brief`, `@details` tags from `.c` and `.h` files
- Creates structured code reference documentation
- Supports cross-module linking

**Usage:**
```bash
python3 scripts/generate_code_reference_md.py -o docs_src/reference-code.md
```

**Input:** `Core/Src/*.c`, `Core/Inc/*.h` (Doxygen comments)  
**Output:** `docs_src/reference-code.md`

---

### `scripts/generate_readme_languages.py`

Generates language-specific README versions (Polish, Russian).

**Features:**
- Translates main README.md to multiple languages
- Maintains parity across language versions
- Handles language-specific formatting

**Usage:**
```bash
python3 scripts/generate_readme_languages.py --source README.md --target README_pl.md
```

**Input:** `README.md` (English master)  
**Output:** `README_pl.md`, `README_ru.md`

---

### `scripts/update_docs_src_index.py`

Maintains documentation structure and index files.

**Features:**
- Updates INDEX.md based on directory structure
- Synchronizes cross-references
- Validates documentation completeness

**Usage:**
```bash
python3 scripts/update_docs_src_index.py
```

**Input:** `docs_src/` directory  
**Output:** `docs_src/INDEX.md` (updated)

---

### `scripts/update_readme.py`

Updates README.md tables and cross-references from source code.

**Features:**
- Parses module information from headers
- Updates "Project Structure" section
- Maintains reference tables

**Usage:**
```bash
python3 scripts/update_readme.py
```

**Input:** `Core/Inc/*.h`, `Core/Src/*.c`  
**Output:** Updated `README.md`

---

## Make Targets

### `make docs`

**Complete documentation build with HTML output.**

```bash
make docs
```

**Process:**
1. Regenerates `docs_src/testing-plan-signing-rng.md`
2. Regenerates `docs_src/reference-code.md` from source headers
3. Builds MkDocs HTML site → `docs/index.html`

**Requirements:**
- MkDocs Material (`pip install -r requirements-docs.txt`)
- Python 3
- All documentation source files

**Output:**
- `docs/index.html` — Main documentation website
- `docs_src/reference-code.md` — Auto-generated code reference

---

### `make docs-md`

**Generate Markdown files only (no HTML).**

```bash
make docs-md
```

**Process:**
1. Regenerates `docs_src/testing-plan-signing-rng.md`
2. Regenerates `docs_src/reference-code.md`
3. Skips MkDocs build

**Output:** Only `.md` files, no HTML website

---

### `make docs-code-md`

**Generate code reference only.**

```bash
make docs-code-md
```

**Process:**
- Parses function/type declarations from `Core/Src/*.c` and `Core/Inc/*.h`
- Extracts Doxygen comments
- Generates structured `docs_src/reference-code.md`

**Script:** `scripts/generate_code_reference_md.py`  
**Output:** `docs_src/reference-code.md`

---

### `make docs-doxygen`

**Full Doxygen processing and README updates.**

```bash
make docs-doxygen
```

**Process:**
1. Runs `doxygen Doxyfile`
2. Generates XML + HTML in `docs_doxygen/`
3. Updates README.md module references
4. Synchronizes docs_src/ indexes

**Output:**
- `docs_doxygen/html/` — Doxygen HTML documentation
- `docs_doxygen/xml/` — Doxygen XML (for further processing)
- Updated `README.md` — Module reference tables

---

### `make docs-serve`

**Local MkDocs live server.**

```bash
make docs-serve
```

**Features:**
- Starts local HTTP server on port 8000
- Live reload on file changes
- Full documentation preview

**Access:** `http://localhost:8000`

**Exit:** Press Ctrl+C

---

## Typical Workflows

### Update Code Reference

When you add new functions or modify headers:

```bash
make docs-code-md
```

This regenerates the code reference without full HTML rebuild.

---

### Full Documentation Build

For complete documentation with HTML:

```bash
make docs
```

This generates:
- Updated code reference
- HTML website in `docs/`
- Ready to deploy

---

### Local Preview

For development and testing:

```bash
make docs-serve
```

Then visit `http://localhost:8000` in your browser.

---

### Language Synchronization

After updating README.md with new sections:

```bash
python3 scripts/generate_readme_languages.py
```

This ensures Polish and Russian versions stay synchronized with the English master.

---

## Configuration Files

### `Doxyfile`

Doxygen configuration for C code parsing.

**Key settings:**
- Input directories: `Core/Inc/`, `Core/Src/`
- Output format: HTML, XML
- Tag processing for cross-references

---

### `mkdocs.yml`

MkDocs configuration for documentation website.

**Key settings:**
- Theme: Material
- Navigation structure: `docs_src/` organization
- Extensions: code highlighting, tables, footnotes

---

### `requirements-docs.txt`

Python dependencies for documentation generation.

**Includes:**
- `mkdocs` — Static site generator
- `mkdocs-material` — Modern theme
- Python utilities for code parsing

**Install:**
```bash
pip install -r requirements-docs.txt
```

---

## Troubleshooting

### MkDocs not found
```bash
# Install documentation dependencies
python3 -m venv .venv-docs
source .venv-docs/bin/activate  # or: .\.venv-docs\Scripts\activate (Windows)
pip install -r requirements-docs.txt
```

### Code reference not updating
```bash
# Force regeneration
rm docs_src/reference-code.md
make docs-code-md
```

### Doxygen errors
```bash
# Check Doxyfile configuration
doxygen Doxyfile
# Review Doxygen output in docs_doxygen/
```

---

## Best Practices

1. **Keep Doxygen comments current** — They feed the auto-generated reference
2. **Test locally before commit** — Use `make docs-serve` to preview
3. **Synchronize languages** — After README updates, regenerate translations
4. **Validate links** — Check for broken cross-references in HTML output
5. **Version control** — Commit generated files to maintain documentation history

---

## Related Files

- **Source:** `Core/Src/*.c`, `Core/Inc/*.h`
- **Configuration:** `Doxyfile`, `mkdocs.yml`
- **Scripts:** `scripts/generate_*.py`, `scripts/update_*.py`
- **Output:** `docs/`, `docs_doxygen/`, `docs_src/reference-code.md`
- **Dependencies:** `requirements-docs.txt`

---

**Last Updated:** 2026-03-20
