# Utrzymanie dokumentacji

## Pliki generowane

| Wynik | Polecenie |
|-------|-----------|
| `documentation/generated/reference-code.md` | `make docs-code-md` lub `python3 scripts/generate_code_reference_md.py -o documentation/generated/reference-code.md` |
| `documentation/generated/testing-plan-signing-rng.md` | `python3 scripts/test_plan_signing_rng.py --write documentation/generated/testing-plan-signing-rng.md` |

## Strona HTML (opcjonalnie)

```bash
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r requirements-docs.txt
make docs    # generatory + mkdocs build → site/
```

Wyjście: **`site/index.html`** (ignorowane przez git). Konfiguracja: `mkdocs.yml` w katalogu głównym repozytorium.

## Doxygen

```bash
make docs-doxygen
```

Tworzy `docs_doxygen/html` i może odświeżać sekcje README przez `scripts/update_readme.py` oraz warianty językowe przez `scripts/generate_readme_languages.py`.

## Styl komentarzy w C

Używaj `@file`, `@brief`, `@details` w nagłówkach, aby `generate_code_reference_md.py` mógł wyciągnąć ustrukturyzowane opisy.
