# Language Audit - Quick Start Guide

## 🎯 The Problem

30 out of 32 modules in `docs_src/` are still in **Russian** when they should be in **English**.

```
Current state:  docs_src/task_sign.md        → 🇷🇺 Russian (wrong!)
Target state:   docs_src/task_sign.md        → 🇬🇧 English (correct!)
                docs_src/task_sign_ru.md     → 🇷🇺 Russian (preserve original)
                docs_src/task_sign_pl.md     → 🇵🇱 Polish (new translation)
```

**Compliance: 6% (2/32 modules)**

---

## ✅ The Solution

### Phase 1: Tier 1 Modules (5 critical modules - 4-6 hours)

Translate these in order:
1. `docs_src/task_sign.md` (82 lines)
2. `docs_src/crypto_wallet.md` (71 lines)
3. `docs_src/task_net.md` (78 lines)
4. `docs_src/wallet_shared.md` (61 lines)
5. `docs_src/task_security.md` (63 lines)

**Result after Phase 1: 25% compliance (7/32 modules)**

### Phase 2: Tier 2+3 (25 supporting modules - 20-25 hours)

Remaining modules in batches.

**Result after Phase 2: 100% compliance (32/32 modules)**

---

## 🚀 How to Fix ONE Module (Example: task_sign)

### Step 1: Read Russian File
```bash
cat docs_src/task_sign.md
```

### Step 2: Translate to English

Read the Russian content and manually translate to English. Key things to keep:

✅ Keep these as-is:
- Doxygen directives: `\page`, `\related`
- Code examples
- Technical terms: SHA-256, ECDSA, BIP-39, FreeRTOS, LwIP, STM32, WebUSB
- Table formatting

🔄 Translate these:
- All regular text
- `<brief>` tags
- Section headers
- Explanations

### Step 3: Save English Base File

Replace `docs_src/task_sign.md` with English content.

### Step 4: Create Russian Version

Copy the current (Russian) content to `docs_src/task_sign_ru.md`.

### Step 5: Create Polish Version

Translate the English content to Polish and save as `docs_src/task_sign_pl.md`.

### Step 6: Verify

```bash
python3 scripts/check_language_compliance.py | grep task_sign
```

Expected output:
```
task_sign
─────────────────────────────────────────────────────────────────
  🇬🇧 EN: ✅ OK
  🇷🇺 RU: ✅ OK
  🇵🇱 PL: ✅ OK
```

### Step 7: Commit

```bash
git add docs_src/task_sign*
git commit -m "Tier 1: Translate task_sign to EN/RU/PL"
```

---

## 📋 Workflow Summary

For each of the 5 Tier 1 modules:

```
1. Read Russian (5 min)
2. Translate to English (15 min)
3. Save as 3 versions (5 min)
4. Verify (2 min)
5. Commit (2 min)

Total per module: ~30 minutes
Total for 5 modules: ~2.5-4 hours
```

---

## 🛠️ Tools Available

### Compliance Checker
```bash
python3 scripts/check_language_compliance.py
```

Shows which modules need work and what's missing.

### README Updater
```bash
make docs-doxygen
```

Automatically updates README.md with module descriptions.

---

## 📚 Detailed Documentation

After quick start, read these for deeper understanding:

1. **COMPLIANCE_AUDIT_SUMMARY.md** — Executive overview
2. **LANGUAGE_COMPLIANCE_AUDIT.md** — Module-by-module breakdown
3. **AUDIT_RECOMMENDATIONS.md** — Decision framework and rationale
4. **LANGUAGE_FIX_STRATEGY.md** — Detailed implementation plan

---

## 📊 Progress Tracking

Track your progress as you go:

```
Session 1 - Tier 1:
  ☐ task_sign       → ✅ Done (3 files: EN/RU/PL)
  ☐ crypto_wallet   → ✅ Done (3 files: EN/RU/PL)
  ☐ task_net        → ✅ Done (3 files: EN/RU/PL)
  ☐ wallet_shared   → ✅ Done (3 files: EN/RU/PL)
  ☐ task_security   → ✅ Done (3 files: EN/RU/PL)
  
  Compliance: 6% → 25% ✅

Session 2 - Tier 2:
  ☐ task_display, task_display_minimal, task_user, task_io
  ☐ tx_request_validate, app_ethernet, app_ethernet_cw
  ☐ usb_device, usb_webusb, time_service
  
  Compliance: 25% → 63% ✅

Session 3 - Tier 3:
  ☐ Remaining 15 hardware/config modules
  
  Compliance: 63% → 100% ✅
```

---

## ⚠️ Important Reminders

### DO
✅ Keep Doxygen directives (`\page`, `\related`)  
✅ Keep code examples unchanged  
✅ Keep technical term abbreviations (SHA-256, ECDSA, etc.)  
✅ Keep English in code comments (separate files)  
✅ Use git commit messages like: "Tier 1: Translate MODULE to EN/RU/PL"  

### DON'T
❌ Translate code comments (they're in different files)  
❌ Change formatting or markdown structure  
❌ Modify line numbers or file structure  
❌ Commit without running compliance checker first  

---

## 🎯 Immediate Next Steps

### Right Now
1. Read this file (you're doing it!)
2. Run compliance checker to see current state:
   ```bash
   python3 scripts/check_language_compliance.py
   ```

### In Next Session (4-6 hours)
1. Open `docs_src/task_sign.md`
2. Follow the "How to Fix ONE Module" workflow above
3. Repeat for 4 more Tier 1 modules
4. Run full compliance check
5. Commit all changes

### Victory Condition
```
Overall: 7/32 modules (22%) → Ready for Phase 2
```

---

## ❓ FAQ

**Q: Can I do all modules at once?**  
A: Not recommended. Start with Tier 1 (5) to establish quality baseline.

**Q: What if I make a mistake?**  
A: Git preserves history. Use `git diff` to review changes. Can revert if needed.

**Q: Do I need to translate code?**  
A: No. Only translate Markdown documentation. Code stays in English.

**Q: How long does each module take?**  
A: 30-45 minutes per module (with experience, faster on later modules).

**Q: Is Polish translation required?**  
A: Yes, it's part of the multi-language strategy. But English base is priority.

**Q: What tools do I need?**  
A: Just Python 3 and git. Everything else is already set up.

---

## 🏁 Let's Go!

Ready to start? Pick a module and begin with the "How to Fix ONE Module" workflow.

**Recommended first module: `task_sign.md`** (most critical for understanding signing pipeline)

```bash
# See current state
cat docs_src/task_sign.md | head -20

# After translating, verify:
python3 scripts/check_language_compliance.py | grep task_sign

# Then commit
git add docs_src/task_sign*
git commit -m "Tier 1: Translate task_sign to EN/RU/PL"
```

Good luck! 🚀

---

**Status:** 🟡 Ready to Begin  
**Priority:** 🔴 CRITICAL - Blocks Phase 2  
**Timeline:** 4-6 hours for Tier 1  
**Target:** 25% compliance after Tier 1
