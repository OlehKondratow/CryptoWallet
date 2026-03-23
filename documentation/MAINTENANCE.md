# Documentation maintenance

## Generated files

| Output | Command |
|--------|---------|
| `documentation/generated/reference-code.md` | `make docs-code-md` or `python3 scripts/generate_code_reference_md.py -o documentation/generated/reference-code.md` |
| `documentation/generated/testing-plan-signing-rng.md` | `python3 scripts/test_plan_signing_rng.py --write documentation/generated/testing-plan-signing-rng.md` |

## HTML site (optional)

```bash
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r requirements-docs.txt
make docs    # runs generators + mkdocs build → site/
```

Output: **`site/index.html`** (ignored by git). Config: `mkdocs.yml` at repo root.

## Doxygen

```bash
make docs-doxygen
```

Produces `docs_doxygen/html` and may refresh README sections via `scripts/update_readme.py` and language variants via `scripts/generate_readme_languages.py`.

## Comment style in C

Use `@file`, `@brief`, `@details` in headers so `generate_code_reference_md.py` can extract structured blurbs.
