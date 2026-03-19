# Multi-Language Documentation Guide

This project provides documentation in **English** (default), **Russian**, and **Polish**.

## Naming Convention

### README Files
- `README.md` — English (main)
- `README_ru.md` — Russian
- `README_pl.md` — Polish

### Documentation Files (docs_src)
- `docs_src/filename.md` — English (main)
- `docs_src/filename_ru.md` — Russian
- `docs_src/filename_pl.md` — Polish

## Current Status

### ✅ Translated
- README.md (EN, RU, PL)
- docs_src/main.md (EN, RU, PL)

### 📋 To Translate (Next Priority)
The following files should be translated for completeness:

**Tier 1 (Core):**
- `docs_src/hw_init.md` — Hardware initialization
- `docs_src/task_sign.md` — Signing pipeline
- `docs_src/crypto_wallet.md` — Cryptography layer
- `docs_src/task_net.md` — Network API
- `docs_src/wallet_shared.md` — Shared IPC

**Tier 2 (Supporting):**
- `docs_src/task_display.md` — Display UI
- `docs_src/task_user.md` — Button handling
- `docs_src/task_io.md` — LED indicators
- `docs_src/app_ethernet_cw.md` — Ethernet/DHCP
- `docs_src/tx_request_validate.md` — Validation

**Tier 3 (Additional):**
- Remaining 22 docs_src/*.md files

## How to Translate

### Manual Translation

1. **Read the English version** (`docs_src/filename.md`)
2. **Open the template** for your target language
3. **Translate only the following sections:**
   - `\page` title and description
   - `<brief>...</brief>` tag content
   - Section headings (## or ###)
   - Descriptive text and explanations
   - Table contents

4. **Keep unchanged:**
   - Code snippets and comments (remain in English)
   - Variable/function names
   - Doxygen commands (`\page`, `\related`, etc.)
   - File paths and references

5. **Save as:** `docs_src/filename_ru.md` or `docs_src/filename_pl.md`

### Example Structure

**English:**
```markdown
# `hw_init.c` + `hw_init.h`

<brief>The `hw_init` module handles board bring-up...</brief>

## Overview
<brief>The `hw_init` module...</brief>

## Abstract
Board initialization performs...

### Key Points
| Register | Purpose |
|---|---|
| `RCC_CFGR` | Clock configuration |
```

**Russian:**
```markdown
# `hw_init.c` + `hw_init.h`

<brief>Модуль `hw_init` отвечает за инициализацию платы...</brief>

## Краткий обзор
<brief>Модуль `hw_init`...</brief>

## Abstract
Инициализация платы выполняет...

### Ключевые точки
| Регистр | Цель |
|---|---|
| `RCC_CFGR` | Конфигурация тактов |
```

## Using Translation Tools

For bulk translation, you can use:

### Option 1: Google Translate API
```bash
pip install google-cloud-translate
export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-key.json
python3 scripts/translate_docs.py --api google
```

### Option 2: Manual with CAT Files
Create translation memory files for consistency:
```
[TERM]
English: hardware initialization
Russian: инициализация аппаратного обеспечения
Polish: inicjalizacja sprzętu
```

## Quality Checklist

Before committing translated files:

- [ ] All section headings translated
- [ ] `<brief>` tags fully translated
- [ ] Code examples unchanged
- [ ] Variable/function names unchanged
- [ ] Doxygen commands intact
- [ ] Markdown formatting preserved
- [ ] File encoding is UTF-8
- [ ] No trailing whitespace

## Commit Message Format

```
Add Russian and Polish translations for X modules

Translate to RU:
- docs_src/hw_init_ru.md
- docs_src/task_sign_ru.md
- docs_src/crypto_wallet_ru.md

Translate to PL:
- docs_src/hw_init_pl.md
- docs_src/task_sign_pl.md
- docs_src/crypto_wallet_pl.md
```

## Navigation

English users see:
```
- [English](README.md)
- [Русский](README_ru.md)
- [Polski](README_pl.md)
```

From any README:
- Click your language link
- All cross-references update to matching language version

## Code Comments Remain English

⚠️ **Important:** Code comments in `.c` and `.h` files **always remain in English**:

✅ Good:
```c
// Initialize clock configuration for STM32H7
void HW_Init(void) {
    // Setup RCC, GPIO, I2C
}
```

❌ Wrong (don't translate code comments):
```c
// Инициализировать конфигурацию тактов для STM32H7
void HW_Init(void) {
    // Настроить RCC, GPIO, I2C
}
```

## Long-Term Strategy

### Phase 1 (Done)
- README.md (EN, RU, PL)
- main.md (EN, RU, PL)

### Phase 2 (Planned)
- Core 5 modules (hw_init, task_sign, crypto_wallet, task_net, wallet_shared)
- All in EN/RU/PL

### Phase 3 (Future)
- Remaining 27 modules
- All in EN/RU/PL

### Phase 4 (Optional)
- Add German (DE)
- Add Chinese (ZH)

## Support for Contributors

If you'd like to contribute translations:

1. **Fork the repository**
2. **Create a branch:** `feature/translate-X-to-RU`
3. **Translate 3-5 files** minimum
4. **Test:** Verify all links work
5. **Create a Pull Request**

Thank you for making this documentation accessible! 🌍
