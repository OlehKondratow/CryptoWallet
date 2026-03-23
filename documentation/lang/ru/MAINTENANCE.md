# Сопровождение документации

## Генерируемые файлы

| Результат | Команда |
|-----------|---------|
| `documentation/generated/reference-code.md` | `make docs-code-md` или `python3 scripts/generate_code_reference_md.py -o documentation/generated/reference-code.md` |
| `documentation/generated/testing-plan-signing-rng.md` | `python3 scripts/test_plan_signing_rng.py --write documentation/generated/testing-plan-signing-rng.md` |

## HTML-сайт (опционально)

```bash
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r requirements-docs.txt
make docs    # генераторы + mkdocs build → site/
```

Выход: **`site/index.html`** (в git не попадает). Конфиг: `mkdocs.yml` в корне репозитория.

## Doxygen

```bash
make docs-doxygen
```

Создаёт `docs_doxygen/html` и может обновлять разделы README через `scripts/update_readme.py` и языковые варианты через `scripts/generate_readme_languages.py`.

## Стиль комментариев в C

Используйте `@file`, `@brief`, `@details` в заголовках, чтобы `generate_code_reference_md.py` мог извлечь структурированные описания.
