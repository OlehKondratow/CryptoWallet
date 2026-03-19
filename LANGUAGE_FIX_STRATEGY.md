# Language Fix Strategy

## Problem Identified

**Critical Issue:** 28 out of 30 `docs_src/*.md` files are still in **Russian**, not English!

- ✅ **2 files correct:** `hw_init.md`, `main.md` (plus their RU/PL versions)
- ❌ **28 files incorrect:** All other modules are in Russian, need translation to English

### Root Cause

Previous documentation audit created Russian-language documentation. The plan to create English base versions (`*.md`) and Russian/Polish translations (`*_ru.md`, `*_pl.md`) was not fully implemented.

### Current State

```
docs_src/task_sign.md          → 🇷🇺 RUSSIAN (should be 🇬🇧 ENGLISH)
docs_src/crypto_wallet.md      → 🇷🇺 RUSSIAN (should be 🇬🇧 ENGLISH)
docs_src/app_ethernet.md       → 🇷🇺 RUSSIAN (should be 🇬🇧 ENGLISH)
... (26 more files in Russian)
```

---

## Solution Strategy

### Phase A: Batch Translate to English (Priority 1)

Translate all 28 Russian files to English in the following priority order:

**Tier 1: Critical Core (5 files)**
1. `task_sign.md` ← `task_sign_ru.md` (then create _ru, _pl)
2. `crypto_wallet.md` ← `crypto_wallet_ru.md` (then create _ru, _pl)
3. `task_net.md` ← `task_net_ru.md` (then create _ru, _pl)
4. `wallet_shared.md` ← `wallet_shared_ru.md` (then create _ru, _pl)
5. `task_security.md` ← `task_security_ru.md` (then create _ru, _pl)

**Tier 2: UI & Display (5 files)**
6. `task_display.md` ← translate
7. `task_display_minimal.md` ← translate
8. `task_user.md` ← translate
9. `task_io.md` ← translate
10. `tx_request_validate.md` ← translate

**Tier 3: Network & USB (6 files)**
11. `app_ethernet.md` ← translate
12. `app_ethernet_cw.md` ← translate
13. `usb_device.md` ← translate
14. `usb_webusb.md` ← translate
15. `task_net.md` ← translate (already in Tier 1, skip here)
16. `time_service.md` ← translate

**Tier 4: Hardware & Config (12 files)**
17. `lwipopts.md` ← translate
18. `ssd1306_conf.md` ← translate
19. `stm32h7xx_hal_msp.md` ← translate
20. `stm32h7xx_it.md` ← translate
21. `stm32h7xx_it_systick.md` ← translate
22. `stm32h7xx_it_usb.md` ← translate
23. `usbd_conf.md` ← translate
24. `usbd_conf_cw.md` ← translate
25. `usbd_desc_cw.md` ← translate
26. `memzero.md` ← translate
27. `sha256_minimal.md` ← translate
28. `wallet_seed.md` ← translate

---

## Implementation Plan

### Step 1: Create Base English Files (*.md)

For each file in priority order:

```bash
# Read current Russian content
original_content = read(docs_src/module.md)

# Translate Russian to English
english_content = translate_to_english(original_content)

# Save as new English base
write(docs_src/module.md, english_content)

# Create Russian version (copy from original)
write(docs_src/module_ru.md, original_content)

# Create Polish version (translate to Polish)
polish_content = translate_to_polish(english_content)
write(docs_src/module_pl.md, polish_content)
```

### Step 2: Verify Compliance

Run language compliance check:
```bash
python3 scripts/check_language_compliance.py
```

Expected output:
- All 32 modules should show ✅ OK
- Each module should have 3 files (EN/RU/PL)

### Step 3: Update README.md Index

The `update_docs_src_index.py` script should automatically extract briefs from now-English files and update `README.md`.

---

## Workflow Optimization

### Use Translation Tool

For efficiency, create a batch translation helper:

```python
# scripts/batch_translate_to_english.py
def translate_docs_to_english():
    """
    For each Russian docs_src/*.md file:
    1. Translate to English
    2. Save as *.md (replacing Russian)
    3. Save original as *_ru.md
    4. Create *_pl.md (Polish translation)
    """
    pass
```

### Manual High-Quality Approach

Given the technical content, recommend manual translation with AI assistance:

1. **AI Model:** Use Claude/GPT-4 for technical translation
2. **Quality Check:** Review domain-specific terms (crypto, FreeRTOS, LwIP, etc.)
3. **Format Validation:** Ensure Doxygen directives preserved
4. **Brief Tags:** Translate brief summaries carefully

---

## Expected Results After Fix

### Compliance Report

```
Phase 1 (Complete)
✅ hw_init (EN/RU/PL) — 320 lines
✅ main (EN/RU/PL) — 75 lines

Phase 2 (After fix)
✅ task_sign (EN/RU/PL)
✅ crypto_wallet (EN/RU/PL)
✅ task_net (EN/RU/PL)
✅ wallet_shared (EN/RU/PL)
✅ task_security (EN/RU/PL)

Phase 3 (After fix)
✅ task_display (EN/RU/PL)
✅ task_display_minimal (EN/RU/PL)
✅ task_user (EN/RU/PL)
✅ task_io (EN/RU/PL)
✅ tx_request_validate (EN/RU/PL)

Phase 4 (After fix)
✅ app_ethernet (EN/RU/PL)
✅ app_ethernet_cw (EN/RU/PL)
✅ usb_device (EN/RU/PL)
✅ usb_webusb (EN/RU/PL)
✅ time_service (EN/RU/PL)
✅ (+ 12 more hardware/config files)

Result: 100% compliance — 32 modules × 3 languages = 96 files ✅
```

---

## Timeline Estimate

- **Tier 1 (5 files × 3 versions = 15 files):** 2-3 hours
- **Tier 2 (5 files × 3 versions = 15 files):** 2-3 hours
- **Tier 3 (6 files × 3 versions = 18 files):** 2-3 hours
- **Tier 4 (12 files × 3 versions = 36 files):** 4-5 hours
- **Verification & fixes:** 1 hour

**Total Estimate:** 12-15 hours of focused work

---

## Quick Start

To begin fixing immediately:

```bash
# 1. Check current state
python3 scripts/check_language_compliance.py

# 2. Create English translation of task_sign.md
# (See: TASK_SIGN_TRANSLATION.md for detailed translation)

# 3. Verify fix worked
python3 scripts/check_language_compliance.py | grep task_sign

# 4. Commit Phase 2 progress
git add docs_src/task_sign*.md
git commit -m "Phase 2: Translate task_sign to EN/RU/PL"
```

---

**Status:** 🔴 CRITICAL - 28 files require language correction  
**Impact:** Documentation compliance 6% → Target 100%  
**Next Action:** Begin Tier 1 English translations
