# Doxygen: Short (Brief) vs Detailed comments

## Краткий обзор
У Doxygen нет нативной опции `GENERATE_MARKDOWN`, поэтому этот проект использует **XML output** и скрипты для генерации Markdown и обновления README.
Чтобы корректно разделять **короткое** (`@brief`) и **длинное** (`@details`) описание, в коде применяется единый шаблон комментариев.

## Детальный анализ

### 1. File-level (first block in each `.c` / `.h`)

Use **two blocks** so that "Brief" and "Detailed" are distinct in the generated docs and in the XML (for scripts).

### Recommended layout

```c
/**
 * @file    my_module.c
 * @brief   One-line summary: what this file does (Short Analysis).
 *
 * @details
 *          Detailed analysis: role in the system, data flow, build flags,
 *          links to other modules or docs_src/*.md. Can span multiple
 *          paragraphs. Use @c for code refs.
 */
```

- **@brief** — one line (or one short sentence). Appears in lists and as the "short" block.
- **@details** — everything after the first paragraph in the block, or under an explicit `@details` tag. Appears as the "Detailed Analysis" in HTML and in XML `<detaileddescription>`.

### STM32-style banner (optional)

If you use the `******` banner, keep `@brief` and `@details` as above:

```c
/**
  ******************************************************************************
  * @file    task_net.c
  * @brief   LwIP + HTTP — one-line role (Short).
  ******************************************************************************
  * @details
  *          Paragraph one of detailed analysis.
  *          Paragraph two.
  ******************************************************************************
  */
```

Doxygen still maps the first line after `@brief` to the brief description and the `@details` block to the detailed description.

### 2. Functions

Same idea: **@brief** = short, **@details** = long (or the first paragraph = brief, rest = detailed if you use `JAVADOC_AUTOBRIEF`).

```c
/**
 * @brief   One-line: what the function does (Short).
 *
 * @details Optional longer description, parameters behaviour, return value
 *          details, thread-safety, etc.
 *
 * @param   buf  Input buffer.
 * @param   len  Length in bytes.
 * @return  0 on success, -1 on error.
 */
int my_api(uint8_t *buf, size_t len);
```

### 3. Python (scripts)

Doxygen can parse Python docstrings. For a **short + long** split:

- First line of the docstring = brief.
- Blank line, then the rest = detailed (Doxygen treats it as such when using `PYTHON_DOCSTRING = YES`).

```python
"""One-line summary (Short).

Detailed description: usage, dependencies, links to docs.
"""
```

### 4. What the scripts use

- **update_readme.py** reads Doxygen XML (`docs_doxygen/xml/`), takes **brief** for each file, and fills the "Project Structure" table in `README.md`.
- With `--md-dir`, it also writes one Markdown file per source file with **Brief** and **Detailed** sections from the XML.

Run Doxygen first: `doxygen Doxyfile`, then `python3 scripts/update_readme.py` (and optionally `--md-dir docs_doxygen/md`). Or from repo root: `make docs-doxygen`.

**README.md:** The script looks for a section heading `## Project Structure` or the HTML comment `<!-- DOXYGEN_PROJECT_STRUCTURE -->`. It replaces the table below that heading with the generated Module | Brief table. If the section is missing, it is appended (or the script creates a minimal README).

## Связи
- `Doxyfile` (включает XML/HTML и `MARKDOWN_SUPPORT = YES`).
- `scripts/update_readme.py` (берёт `@brief`/`@details` из Doxygen XML и обновляет таблицу `README.md`).
- `make docs-doxygen` (основной путь: `doxygen Doxyfile` + обновление README).
